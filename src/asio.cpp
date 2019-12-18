#include <fc/asio.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <fc/thread/fibers.hpp>
#include <boost/scope_exit.hpp>

#include <algorithm>
#include <functional>
#include <thread>

namespace fc {
  namespace asio {
    namespace detail {

        template<typename EndpointType, typename IteratorType>
        void resolve_handler( std::shared_ptr< boost::fibers::promise<std::vector<EndpointType> > > p,
                              const boost::system::error_code& ec,
                              IteratorType itr) {
            if( !ec ) {
                std::vector<EndpointType> eps;
                while( itr != IteratorType() ) {
                    eps.push_back(*itr);
                    ++itr;
                }
                p->set_value( eps );
            } else {
                p->set_exception(
                    std::make_exception_ptr( fc::exception(
                        FC_LOG_MESSAGE( error, "process exited with: ${message} ",
                                        ("message", boost::system::system_error(ec).what())) ) ) );
            }
        }
    }

    uint16_t fc::asio::default_io_service_scope::num_io_threads = 0;

    /***
     * @brief set the default number of threads for the io service
     *
     * Sets the number of threads for the io service. This will throw
     * an exception if called more than once.
     *
     * @param num_threads the number of threads
     */
    void default_io_service_scope::set_num_threads(uint16_t num_threads) {
       FC_ASSERT(num_io_threads == 0);
       num_io_threads = num_threads;
    }

    uint16_t default_io_service_scope::get_num_threads() { return num_io_threads; }

    /***
     * Default constructor
     */
    default_io_service_scope::default_io_service_scope()
    {
       io           = new boost::asio::io_service();
       the_work     = new boost::asio::io_service::work(*io);

       if( num_io_threads == 0 )
       {
          // the default was not set by the configuration. Determine a good
          // number of threads. Minimum of 8, maximum of hardware_concurrency
          num_io_threads = std::max( std::thread::hardware_concurrency(), 8U );
       }

       asio_threads.reserve( num_io_threads );
       for( uint16_t i = 0; i < num_io_threads; ++i )
       {
          asio_threads.emplace_back( [i,this]() {
             set_thread_name( "fc::asio worker #" + fc::to_string(i) );
                 while (!io->stopped())
                 {
                    try
                    {
                       io->run();
                    }
                    catch (const fc::exception& e)
                    {
                       elog("Caught unhandled exception in asio service loop: ${e}", ("e", e));
                    }
                    catch (const std::exception& e)
                    {
                       elog("Caught unhandled exception in asio service loop: ${e}", ("e", e.what()));
                    }
                    catch (...)
                    {
                       elog("Caught unhandled exception in asio service loop");
                    }
                 }
          } );
       } // build thread loop
    } // end of constructor

    /***
     * destructor
     */
    default_io_service_scope::~default_io_service_scope()
    {
       delete the_work;
       io->stop();
       for( auto& asio_thread : asio_threads )
       {
          asio_thread.join();
       }
       delete io;
    } // end of destructor

    /***
     * @brief create an io_service
     * @returns the io_service
     */
    boost::asio::io_service& default_io_service() {
        static default_io_service_scope fc_asio_service[1];
        return *fc_asio_service[0].io;
    }

    namespace tcp {
      std::vector<boost::asio::ip::tcp::endpoint> resolve( const std::string& hostname, const std::string& port)
      {
        try
        {
          resolver res( fc::asio::default_io_service() );
          std::shared_ptr< boost::fibers::promise<std::vector<boost::asio::ip::tcp::endpoint> > > p;
          p = std::make_shared< boost::fibers::promise<std::vector<boost::asio::ip::tcp::endpoint> > >();
          auto f = p->get_future();
          res.async_resolve( boost::asio::ip::tcp::resolver::query(hostname,port),
                             std::bind( detail::resolve_handler<boost::asio::ip::tcp::endpoint,resolver_iterator>,
                                        p, std::placeholders::_1, std::placeholders::_2 ) );
          return f.get();
        }
        FC_RETHROW_EXCEPTIONS(warn, "")
      }
    }
    namespace udp {
      std::vector<udp::endpoint> resolve( resolver& r, const std::string& hostname, const std::string& port)
      {
        try
        {
          resolver res( fc::asio::default_io_service() );
          std::shared_ptr< boost::fibers::promise<std::vector<endpoint> > > p;
          p = std::make_shared< boost::fibers::promise<std::vector<endpoint> > >();
          auto f = p->get_future();
          res.async_resolve( resolver::query(hostname,port),
                             std::bind( detail::resolve_handler<endpoint,resolver_iterator>,
                                        p, std::placeholders::_1, std::placeholders::_2 ) );
          return f.get();
        }
        FC_RETHROW_EXCEPTIONS(warn, "")
      }
    }

} } // namespace fc::asio
