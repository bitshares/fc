/**
 *  @file fc/asio.hpp
 *  @brief defines wrappers for boost::asio functions
 */
#pragma once
#include <boost/asio.hpp>
#include <boost/fiber/asio/yield.hpp>

#include <vector>

#include <fc/exception/exception.hpp>
#include <fc/io/iostream.hpp>

namespace fc { 
/**
 *  @brief defines fc wrappers for boost::asio functions.
 */
namespace asio {
    /**
     *  @brief internal implementation types/methods for fc::asio
     */
    namespace detail {
        template<typename C>
        struct non_blocking { 
          bool operator()( C& c ) { return c.non_blocking(); } 
          bool operator()( C& c, bool s ) { c.non_blocking(s); return true; } 
        };

#if WIN32  // windows stream handles do not support non blocking!
	       template<>
         struct non_blocking<boost::asio::windows::stream_handle> { 
	          typedef boost::asio::windows::stream_handle C;
            bool operator()( C& ) { return false; } 
            bool operator()( C&, bool ) { return false; } 
        };
#endif
    } // end of namespace detail

    /***
     * A structure for holding the boost io service and associated
     * threads
     */
    class default_io_service_scope
    {
       public:
          default_io_service_scope();
          ~default_io_service_scope();
          static void     set_num_threads(uint16_t num_threads);
          static uint16_t get_num_threads();
          boost::asio::io_service*          io;
       private:
          std::vector<std::thread>          asio_threads;
          boost::asio::io_service::work*    the_work;
       protected:
          static uint16_t num_io_threads; // marked protected to help with testing
    };

    /**
     * @return the default boost::asio::io_service for use with fc::asio
     * 
     * This IO service is automatically running in its own thread to service asynchronous
     * requests without blocking any other threads.
     */
    boost::asio::io_service& default_io_service();

    template<typename AsyncReadStream>
    size_t read_some( AsyncReadStream& s, char* buffer, size_t length, size_t offset = 0 )
    {
      boost::system::error_code ec;
      std::size_t rlen = s.async_read_some( boost::asio::buffer(buffer + offset, length),
                                            boost::fibers::asio::yield_t()[ec] );
      if ( ec == boost::asio::error::eof) {
         throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      } else if ( ec) {
         throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      }
      return rlen;
    }

    template<typename AsyncReadStream>
    size_t read_some(AsyncReadStream& s, const std::shared_ptr<char>& buffer, size_t length, size_t offset)
    {
      boost::system::error_code ec;
      std::size_t rlen = s.async_read_some( boost::asio::buffer(buffer.get() + offset, length),
                                            boost::fibers::asio::yield_t()[ec] );
      if ( ec == boost::asio::error::eof) {
         throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      } else if ( ec) {
         throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      }
      return rlen;
    }

    template<typename AsyncWriteStream>
    size_t write_some( AsyncWriteStream& s, const char* buffer, size_t length, size_t offset = 0 ) {
      boost::system::error_code ec;
      std::size_t rlen = s.async_write_some( boost::asio::buffer(buffer + offset, length),
                                             boost::fibers::asio::yield_t()[ec] );
      if ( ec == boost::asio::error::eof) {
         throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      } else if ( ec) {
         throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      }
      return rlen;
    }

    template<typename AsyncWriteStream>
    size_t write_some( AsyncWriteStream& s, const std::shared_ptr<const char>& buffer,
                       size_t length, size_t offset ) {
      boost::system::error_code ec;
      std::size_t rlen = s.async_write_some( boost::asio::buffer(buffer.get() + offset, length),
                                             boost::fibers::asio::yield_t()[ec] );
      if ( ec == boost::asio::error::eof) {
         throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      } else if ( ec) {
         throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
      }
      return rlen;
    }

    namespace tcp {
        typedef boost::asio::ip::tcp::endpoint endpoint;
        typedef boost::asio::ip::tcp::resolver::iterator resolver_iterator;
        typedef boost::asio::ip::tcp::resolver resolver;
        std::vector<endpoint> resolve( const std::string& hostname, const std::string& port );

        /** @brief wraps boost::asio::async_accept
          * @post sock is connected
          * @post sock.non_blocking() == true  
          * @throw on error.
          */
        template<typename SocketType, typename AcceptorType>
        void accept( AcceptorType& acc, SocketType& sock ) {
           boost::system::error_code ec;
           acc.async_accept( sock, boost::fibers::asio::yield_t()[ec] );
           if ( ec == boost::asio::error::eof) {
              throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
           } else if ( ec) {
              throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
           }
        }

        /** @brief wraps boost::asio::socket::async_connect
          * @post sock.non_blocking() == true  
          * @throw on error
          */
        template<typename AsyncSocket, typename EndpointType>
        void connect( AsyncSocket& sock, const EndpointType& ep ) {
           boost::system::error_code ec;
           sock.async_connect( ep, boost::fibers::asio::yield_t()[ec] );
           if ( ec == boost::asio::error::eof) {
              throw fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
           } else if ( ec) {
              throw fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) );
           }
        }
    }
    namespace udp {
        typedef boost::asio::ip::udp::endpoint endpoint;
        typedef boost::asio::ip::udp::resolver::iterator resolver_iterator;
        typedef boost::asio::ip::udp::resolver resolver;
        /// @brief resolve all udp::endpoints for hostname:port
        std::vector<endpoint> resolve( resolver& r, const std::string& hostname, 
                                                         const std::string& port );
    }

    template<typename AsyncReadStream>
    class istream : public virtual fc::istream
    {
       public:
          istream( std::shared_ptr<AsyncReadStream> str )
          :_stream( std::move(str) ){}

          virtual size_t readsome( char* buf, size_t len )
          {
             return fc::asio::read_some(*_stream, buf, len);
          }
          virtual size_t readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset )
          {
             return fc::asio::read_some(*_stream, buf, len, offset);
          }
    
       private:
          std::shared_ptr<AsyncReadStream> _stream;
    };

    template<typename AsyncWriteStream>
    class ostream : public virtual fc::ostream
    {
       public:
          ostream( std::shared_ptr<AsyncWriteStream> str )
          :_stream( std::move(str) ){}

          virtual size_t writesome( const char* buf, size_t len )
          {
             return fc::asio::write_some(*_stream, buf, len);
          }
    
          virtual size_t     writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
          {
             return fc::asio::write_some(*_stream, buf, len, offset);
          }
    
          virtual void       close(){ _stream->close(); }
          virtual void       flush() {}
       private:
          std::shared_ptr<AsyncWriteStream> _stream;
    };


} } // namespace fc::asio

