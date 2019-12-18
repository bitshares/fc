#pragma once
#include <functional>
#include <memory>
#include <string>
#include <boost/any.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/http/connection.hpp>
#include <fc/signals.hpp>

namespace fc { namespace http {
   namespace detail {
      class websocket_server_impl;
      class websocket_tls_server_impl;
      class websocket_client_impl;
      class websocket_tls_client_impl;
   } // namespace detail

   class websocket_connection
   {
      public:
         virtual ~websocket_connection(){}
         virtual void send_message( const std::string& message ) = 0;
         virtual void close( int64_t code, const std::string& reason  ){};
         void on_message( const std::string& message ) { _on_message(message); }
         fc::http::reply on_http( const std::string& message ) { return _on_http(message); }

         void on_message_handler( const std::function<void(const std::string&)>& h ) { _on_message = h; }
         void on_http_handler( const std::function<fc::http::reply(const std::string&)>& h ) { _on_http = h; }

         void        set_session_data( boost::any d ){ _session_data = std::move(d); }
         boost::any& get_session_data() { return _session_data; }

         virtual std::string get_request_header(const std::string& key) = 0;

         fc::signal<void()> closed;
      private:
         boost::any                                _session_data;
         std::function<void(const std::string&)>   _on_message;
         std::function<fc::http::reply(const std::string&)> _on_http;
   };
   typedef std::shared_ptr<websocket_connection> websocket_connection_ptr;

   typedef std::function<void(const websocket_connection_ptr&)> on_connection_handler;

   class websocket_server
   {
      public:
         websocket_server();
         ~websocket_server();

         void on_connection( const on_connection_handler& handler);
         void listen( uint16_t port );
         void listen( const fc::ip::endpoint& ep );
         uint16_t get_listening_port();
         void start_accept();

         void stop_listening();
         void close();

      private:
         friend class detail::websocket_server_impl;
         std::unique_ptr<detail::websocket_server_impl> my;
   };


   class websocket_tls_server
   {
      public:
         websocket_tls_server( const std::string& server_pem = std::string(),
                           const std::string& ssl_password = std::string());
         ~websocket_tls_server();

         void on_connection( const on_connection_handler& handler);
         void listen( uint16_t port );
         void listen( const fc::ip::endpoint& ep );
         void start_accept();

      private:
         friend class detail::websocket_tls_server_impl;
         std::unique_ptr<detail::websocket_tls_server_impl> my;
   };

   class websocket_client
   {
      public:
         websocket_client( const std::string& ca_filename = "_default" );
         ~websocket_client();

         websocket_connection_ptr connect( const std::string& uri );
         websocket_connection_ptr secure_connect( const std::string& uri );

         void close();
         void synchronous_close();
      private:
         std::unique_ptr<detail::websocket_client_impl> my;
         std::unique_ptr<detail::websocket_tls_client_impl> smy;
   };

} }
