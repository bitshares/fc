#pragma once
#include <fc/rpc/api_connection.hpp>
#include <fc/variant.hpp>

#include <boost/fiber/future.hpp>

#include <iostream>
#include <string>

namespace fc { namespace rpc {

   /**
    *  Provides a simple wrapper for RPC calls to a given interface.
    */
   class cli : public api_connection
   {
      public:
         cli( uint32_t max_depth ) : api_connection(max_depth) {}
         ~cli();

         virtual variant send_call( api_id_type api_id, std::string method_name, variants args = variants() );
         virtual variant send_callback( uint64_t callback_id, variants args = variants() );
         virtual void    send_notice( uint64_t callback_id, variants args = variants() );

         void start();
         void stop();
         void cancel();
         void wait();
         void format_result( const std::string& method, std::function<std::string(variant,const variants&)> formatter);

         virtual void getline( const std::string& prompt, std::string& line );

         void set_prompt( const std::string& prompt );

         void set_regex_secret( const std::string& expr );

      private:
         void run();

         std::string _prompt = ">>>";
         std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
         boost::fibers::future<void> _run_complete;
   };
} } 
