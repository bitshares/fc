#include <fc/exception/exception.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/thread/thread.hpp>
#include <fc/variant.hpp>

#include <boost/thread/mutex.hpp>

#include <fstream>
#include <iomanip>
#include <queue>
#include <sstream>
#include <iostream>

namespace fc {

   class file_appender::impl
   {
      public:
         config                     cfg;
         std::ofstream              out;
         boost::mutex               slock;

      private:
         future<void>               _deletion_task;
         boost::atomic<int64_t>     _current_file_number;
         const int64_t              _interval_seconds;
         time_point                 _next_file_time;

      public:
         impl( const config& c) : cfg( c ), _interval_seconds( cfg.rotation_interval.to_seconds() )
         {
            try
            {
               fc::create_directories(cfg.filename.parent_path());

               if( cfg.rotate )
               {
                  FC_ASSERT( cfg.rotation_interval >= seconds( 1 ) );
                  FC_ASSERT( cfg.rotation_limit >= cfg.rotation_interval );

                  rotate_files( true );
                  delete_files();
               } else {
                  out.open( cfg.filename.string(), std::ios_base::app );
                  FC_ASSERT( !out.fail() && !out.bad(), "Failed to open file '${f}'",
                             ("f",cfg.filename.string()) );
               }
            }
            catch( ... )
            {
               std::cerr << "error opening log file: " << cfg.filename.preferred_string() << "\n";
            }
         }

         ~impl()
         {
            try
            {
              _deletion_task.cancel_and_wait("file_appender is destructing");
            }
            catch( ... )
            {
            }
         }

         void rotate_files( bool initializing = false )
         {
             if( !cfg.rotate ) return;

             fc::time_point now = time_point::now();
             if( now < _next_file_time ) return;

             int64_t new_file_number = now.sec_since_epoch() / _interval_seconds;
             if( initializing )
                _current_file_number.store( new_file_number );
             else
             {
                int64_t prev_file_number = _current_file_number.load();
                if( prev_file_number >= new_file_number ) return;
                if( !_current_file_number.compare_exchange_weak( prev_file_number, new_file_number ) ) return;
             }
             fc::time_point_sec start_time = time_point_sec( (uint32_t)(new_file_number * _interval_seconds) );
             _next_file_time = start_time + _interval_seconds;
             string timestamp_string = start_time.to_non_delimited_iso_string();
             fc::path link_filename = cfg.filename;
             fc::path log_filename = link_filename.parent_path() / (link_filename.filename().string() + "." + timestamp_string);

             {
               fc::scoped_lock<boost::mutex> lock( slock );

               if( !initializing )
               {
                   out.flush();
                   out.close();
               }
               remove_all(link_filename);  // on windows, you can't delete the link while the underlying file is opened for writing
               out.open( log_filename.string(), std::ios_base::app );
               if( out.fail() || out.bad() )
                  std::cerr << "Failed to open log file '" << cfg.filename.string() << "'\n";
               create_hard_link(log_filename, link_filename);
             }
         }

         void delete_files()
         {
             /* Delete old log files */
             auto current_file = _current_file_number.load();
             fc::time_point_sec start_time = time_point_sec( (uint32_t)(current_file * _interval_seconds) );
             fc::time_point limit_time = time_point::now() - cfg.rotation_limit;
             fc::path link_filename = cfg.filename;
             string link_filename_string = link_filename.filename().string();
             directory_iterator itr(link_filename.parent_path());
             string timestamp_string = start_time.to_non_delimited_iso_string();
             for( ; itr != directory_iterator(); itr++ )
             {
                 try
                 {
                     string current_filename = itr->filename().string();
                     if( current_filename.compare(0, link_filename_string.size(), link_filename_string) != 0
                            || current_filename.size() <= link_filename_string.size() + 1 )
                        continue;
                     string current_timestamp_str = current_filename.substr(link_filename_string.size() + 1,
                                                                            timestamp_string.size());
                     fc::time_point_sec current_timestamp = fc::time_point_sec::from_iso_string( current_timestamp_str );
                     if( current_timestamp < start_time
                            && ( current_timestamp < limit_time || file_size( current_filename ) <= 0 ) )
                     {
                        remove_all( *itr );
                        continue;
                     }
                 }
                 catch (const fc::canceled_exception&)
                 {
                     throw;
                 }
                 catch( ... )
                 {
                 }
             }
             uint64_t next_file = time_point::now().sec_since_epoch() / _interval_seconds + 1;
             _deletion_task = schedule( [this]() { delete_files(); },
                                        fc::time_point_sec( next_file * _interval_seconds),
                                        "delete_files(3)" );
         }
   };

   file_appender::config::config(const fc::path& p) :
     format( "${timestamp} ${thread_name} ${context} ${file}:${line} ${method} ${level}]  ${message}" ),
     filename(p),
     flush(true),
     rotate(false)
   {}

   file_appender::file_appender( const variant& args ) :
     my( new impl( args.as<config>( FC_MAX_LOG_OBJECT_DEPTH ) ) )
   {}

   file_appender::~file_appender(){}

   // MS THREAD METHOD  MESSAGE \t\t\t File:Line
   void file_appender::log( const log_message& m )
   {
      my->rotate_files();

      std::stringstream line;
      line << string(m.get_context().get_timestamp()) << " ";
      line << std::setw( 21 ) << (m.get_context().get_thread_name().substr(0,9) + string(":") + m.get_context().get_task_name()).c_str() << " ";

      string method_name = m.get_context().get_method();
      // strip all leading scopes...
      if( method_name.size() )
      {
         uint32_t p = 0;
         for( uint32_t i = 0;i < method_name.size(); ++i )
         {
             if( method_name[i] == ':' ) p = i;
         }

         if( method_name[p] == ':' )
           ++p;
         line << std::setw( 20 ) << m.get_context().get_method().substr(p,20).c_str() <<" ";
      }

      line << "] ";
      std::string message = fc::format_string( m.get_format(), m.get_data(), my->cfg.max_object_depth );
      line << message.c_str();

      {
        fc::scoped_lock<boost::mutex> lock( my->slock );
        my->out << line.str() << "\t\t\t" << m.get_context().get_file() << ":" << m.get_context().get_line_number() << "\n";
        if( my->cfg.flush )
          my->out.flush();
      }
   }

} // fc
