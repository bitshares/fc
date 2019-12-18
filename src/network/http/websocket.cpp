#include <fc/network/http/websocket.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>

#ifdef HAS_ZLIB
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#else
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#endif

#include <fc/io/json.hpp>
#include <fc/optional.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/variant.hpp>
#include <fc/asio.hpp>
#include <fc/thread/async.hpp>

#if WIN32
#include <wincrypt.h>
#endif

#ifdef DEFAULT_LOGGER
# undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "rpc"

namespace fc { namespace http {

   namespace detail {
#if WIN32
      // taken from https://stackoverflow.com/questions/39772878/reliable-way-to-get-root-ca-certificates-on-windows/40710806
      static void add_windows_root_certs(boost::asio::ssl::context &ctx)
      {
         HCERTSTORE hStore = CertOpenSystemStore( 0, "ROOT" );
         if( hStore == NULL )
            return;

         X509_STORE *store = X509_STORE_new();
         PCCERT_CONTEXT pContext = NULL;
         while( (pContext = CertEnumCertificatesInStore( hStore, pContext )) != NULL )
         {
            X509 *x509 = d2i_X509( NULL, (const unsigned char **)&pContext->pbCertEncoded,
                                   pContext->cbCertEncoded);
            if( x509 != NULL )
            {
               X509_STORE_add_cert( store, x509 );
               X509_free( x509 );
            }
         }

         CertFreeCertificateContext( pContext );
         CertCloseStore( hStore, 0 );

         SSL_CTX_set_cert_store( ctx.native_handle(), store );
      }
#endif
      struct asio_with_stub_log : public websocketpp::config::asio {

          typedef asio_with_stub_log type;
          typedef asio base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          /// Custom Logging policies
          /*typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::elevel> elog_type;
          typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::alevel> alog_type;
          */
          //typedef base::alog_type alog_type;
          //typedef base::elog_type elog_type;
          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef websocketpp::transport::asio::basic_socket::endpoint
                  socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;

          static const long timeout_open_handshake = 0;

       // permessage_compress extension
       struct permessage_deflate_config {};
#ifdef HAS_ZLIB
       typedef websocketpp::extensions::permessage_deflate::enabled <permessage_deflate_config> permessage_deflate_type;
#else
       typedef websocketpp::extensions::permessage_deflate::disabled <permessage_deflate_config> permessage_deflate_type;
#endif

      };
      struct asio_tls_with_stub_log : public websocketpp::config::asio_tls {

          typedef asio_with_stub_log type;
          typedef asio_tls base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          /// Custom Logging policies
          /*typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::elevel> elog_type;
          typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::alevel> alog_type;
          */
          //typedef base::alog_type alog_type;
          //typedef base::elog_type elog_type;
          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef websocketpp::transport::asio::tls_socket::endpoint socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;

          static const long timeout_open_handshake = 0;
      };
      struct asio_tls_stub_log : public websocketpp::config::asio_tls {
         typedef asio_tls_stub_log type;
         typedef asio_tls base;

         typedef base::concurrency_type concurrency_type;

         typedef base::request_type request_type;
         typedef base::response_type response_type;

         typedef base::message_type message_type;
         typedef base::con_msg_manager_type con_msg_manager_type;
         typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

         //typedef base::alog_type alog_type;
         //typedef base::elog_type elog_type;
         typedef websocketpp::log::stub elog_type;
         typedef websocketpp::log::stub alog_type;

         typedef base::rng_type rng_type;

         struct transport_config : public base::transport_config {
         typedef type::concurrency_type concurrency_type;
         typedef type::alog_type alog_type;
         typedef type::elog_type elog_type;
         typedef type::request_type request_type;
         typedef type::response_type response_type;
         typedef websocketpp::transport::asio::tls_socket::endpoint socket_type;
         };

         typedef websocketpp::transport::asio::endpoint<transport_config>
         transport_type;
      };





      using websocketpp::connection_hdl;
      typedef websocketpp::server<asio_with_stub_log>  websocket_server_type;
      typedef websocketpp::server<asio_tls_stub_log>   websocket_tls_server_type;

      template<typename T>
      class websocket_connection_impl : public websocket_connection
      {
         public:
            websocket_connection_impl( T con )
            :_ws_connection(con){
            }

            virtual ~websocket_connection_impl()
            {
            }

            virtual void send_message( const std::string& message )override
            {
               idump((message));
               //std::cerr<<"send: "<<message<<"\n";
               auto ec = _ws_connection->send( message );
               FC_ASSERT( !ec, "websocket send failed: ${msg}", ("msg",ec.message() ) );
            }
            virtual void close( int64_t code, const std::string& reason  )override
            {
               _ws_connection->close(code,reason);
            }

            virtual std::string get_request_header(const std::string& key)override
            {
              return _ws_connection->get_request_header(key);
            }

            T _ws_connection;
      };

      typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

      class websocket_server_impl
      {
         public:
            websocket_server_impl()
            :_server_thread_id( std::this_thread::get_id() )
            {

               _server.clear_access_channels( websocketpp::log::alevel::all );
               _server.init_asio(&fc::asio::default_io_service());
               _server.set_reuse_addr(true);
               _server.set_open_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       auto new_con = std::make_shared<websocket_connection_impl<websocket_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       _connections[hdl] = new_con;
                       lock.unlock();
                       _on_connection( new_con );
                    }, _server_thread_id ).wait();
               });
               _server.set_message_handler( [this]( connection_hdl hdl, websocket_server_type::message_ptr msg ){
                    fc::async( [this,&hdl,&msg](){
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       auto current_con = _connections.find(hdl);
                       assert( current_con != _connections.end() );
                       wdump(("server")(msg->get_payload()));
                       auto payload = msg->get_payload();
                       std::shared_ptr<websocket_connection> con = current_con->second;
                       lock.unlock();
                       ++_pending_messages;
                       auto f = fc::async([this,con,payload](){
                          if( _pending_messages )
                             --_pending_messages;
                          con->on_message( payload );
                       }, _server_thread_id );
                       if( _pending_messages > 100 ) 
                         f.wait();
                    }, _server_thread_id ).wait();
               });

               _server.set_socket_init_handler( [&](websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket& s ) {
                      boost::asio::ip::tcp::no_delay option(true);
                      s.lowest_layer().set_option(option);
               } );

               _server.set_http_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       auto current_con = std::make_shared<websocket_connection_impl<websocket_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       _on_connection( current_con );

                       auto con = _server.get_con_from_hdl(hdl);
                       con->defer_http_response();
                       std::string request_body = con->get_request_body();
                       wdump(("server")(request_body));

                       fc::async([current_con, request_body, con] {
                          fc::http::reply response = current_con->on_http(request_body);
                          idump( (response) );
                          con->set_body( std::move( response.body_as_string ) );
                          con->set_status( websocketpp::http::status_code::value(response.status) );
                          con->send_http_response();
                          current_con->closed();
                       }, _server_thread_id, "call on_http");
                    }, _server_thread_id ).wait();
               });

               _server.set_close_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       if( _connections.find(hdl) != _connections.end() )
                       {
                          _connections[hdl]->closed();
                          _connections.erase( hdl );
                       }
                       else
                       {
                            wlog( "unknown connection closed" );
                       }
                       if( _connections.empty() )
                       {
                          _closed = true;
                          cv.notify_all();
                       }
                    }, _server_thread_id ).wait();
               });

               _server.set_fail_handler( [this]( connection_hdl hdl ){
                    if( _server.is_listening() )
                    {
                       fc::async( [this,&hdl](){
                          std::unique_lock<boost::fibers::mutex> lock(_mtx);
                          if( _connections.find(hdl) != _connections.end() )
                          {
                             _connections[hdl]->closed();
                             _connections.erase( hdl );
                          }
                          else
                          {
                            wlog( "unknown connection failed" );
                          }
                          if( _connections.empty() )
                          {
                             _closed = true;
                             cv.notify_all();
                          }
                       }, _server_thread_id ).wait();
                    }
               });
            }
            ~websocket_server_impl()
            {
               std::unique_lock<boost::fibers::mutex> lock(_mtx);
               if( _server.is_listening() )
                  _server.stop_listening();

               if( _connections.size() )
                  _closed = false;

               auto cpy_con = _connections;
               for( auto item : cpy_con )
                  _server.close( item.first, 0, "server exit" );

               if( cpy_con.size() )
                  cv.wait( lock, [this] () { return _closed; } );
            }

            typedef std::map<connection_hdl, websocket_connection_ptr,std::owner_less<connection_hdl> > con_map;

            con_map                  _connections;
            std::thread::id          _server_thread_id;
            websocket_server_type    _server;
            on_connection_handler    _on_connection;
            uint32_t                 _pending_messages = 0;

            boost::fibers::mutex _mtx;
            boost::fibers::condition_variable cv;
            bool _closed = false;
      };

      class websocket_tls_server_impl
      {
         public:
            websocket_tls_server_impl( const string& server_pem, const string& ssl_password )
            :_server_thread_id( std::this_thread::get_id() )
            {
               //if( server_pem.size() )
               {
                  _server.set_tls_init_handler( [this,server_pem,ssl_password]( websocketpp::connection_hdl hdl ) -> context_ptr {
                        context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
                        try {
                           ctx->set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::no_sslv3 |
                           boost::asio::ssl::context::single_dh_use);
                           ctx->set_password_callback([ssl_password](std::size_t max_length, boost::asio::ssl::context::password_purpose){ return ssl_password;});
                           ctx->use_certificate_chain_file(server_pem);
                           ctx->use_private_key_file(server_pem, boost::asio::ssl::context::pem);
                        } catch (std::exception& e) {
                           std::cout << e.what() << std::endl;
                        }
                        return ctx;
                  });
               }

               _server.clear_access_channels( websocketpp::log::alevel::all );
               _server.init_asio(&fc::asio::default_io_service());
               _server.set_reuse_addr(true);
               _server.set_open_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       auto new_con = std::make_shared<websocket_connection_impl<websocket_tls_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       _connections[hdl] = new_con;
                       lock.unlock();
                       _on_connection( new_con );
                    }, _server_thread_id ).wait();
               });
               _server.set_message_handler( [this]( connection_hdl hdl, websocket_server_type::message_ptr msg ){
                    fc::async( [this,&hdl,&msg](){
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       auto current_con = _connections.find(hdl);
                       assert( current_con != _connections.end() );
                       auto received = msg->get_payload();
                       std::shared_ptr<websocket_connection> con = current_con->second;
                       lock.unlock();
                       fc::async( [con,received] () { con->on_message( received ); }, _server_thread_id );
                    }, _server_thread_id ).wait();
               });

               _server.set_http_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       auto current_con = std::make_shared<websocket_connection_impl<websocket_tls_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       try{
                          _on_connection( current_con );

                          auto con = _server.get_con_from_hdl(hdl);
                          wdump(("server")(con->get_request_body()));
                          auto response = current_con->on_http( con->get_request_body() );
                          idump((response));
                          con->set_body( std::move( response.body_as_string ) );
                          con->set_status( websocketpp::http::status_code::value( response.status ) );
                       } catch ( const fc::exception& e )
                       {
                         edump((e.to_detail_string()));
                       }
                       current_con->closed();

                    }, _server_thread_id ).wait();
               });

               _server.set_close_handler( [this]( connection_hdl hdl ){
                    fc::async( [this,&hdl](){
                       std::unique_lock<boost::fibers::mutex> lock(_mtx);
                       _connections[hdl]->closed();
                       _connections.erase( hdl );
                       if( _connections.empty() )
                       {
                          _closed = true;
                          cv.notify_all();
                       }
                    }, _server_thread_id ).wait();
               });

               _server.set_fail_handler( [this]( connection_hdl hdl ){
                    if( _server.is_listening() )
                    {
                       fc::async( [this,&hdl](){
                          std::unique_lock<boost::fibers::mutex> lock(_mtx);
                          if( _connections.find(hdl) != _connections.end() )
                          {
                             _connections[hdl]->closed();
                             _connections.erase( hdl );
                          }
                          if( _connections.empty() )
                          {
                             _closed = true;
                             cv.notify_all();
                          }
                       }, _server_thread_id ).wait();
                    }
               });
            }
            ~websocket_tls_server_impl()
            {
               std::unique_lock<boost::fibers::mutex> lock(_mtx);
               if( _server.is_listening() )
                  _server.stop_listening();

               if( _connections.size() )
                  _closed = false;

               auto cpy_con = _connections;
               for( auto item : cpy_con )
                  _server.close( item.first, 0, "server exit" );

               if( cpy_con.size() )
                  cv.wait( lock, [this] () { return _closed; } );
            }

            typedef std::map<connection_hdl, websocket_connection_ptr,std::owner_less<connection_hdl> > con_map;

            con_map                     _connections;
            std::thread::id             _server_thread_id;
            websocket_tls_server_type   _server;
            on_connection_handler       _on_connection;

            boost::fibers::mutex _mtx;
            boost::fibers::condition_variable cv;
            bool _closed = false;
      };













      typedef websocketpp::client<asio_with_stub_log> websocket_client_type;
      typedef websocketpp::client<asio_tls_stub_log> websocket_tls_client_type;

      typedef websocket_client_type::connection_ptr  websocket_client_connection_type;
      typedef websocket_tls_client_type::connection_ptr  websocket_tls_client_connection_type;
      using websocketpp::connection_hdl;

      template<typename T>
      class generic_websocket_client_impl
      {
         public:
            generic_websocket_client_impl()
            :_client_thread_id( std::this_thread::get_id() )
            {
                _client.clear_access_channels( websocketpp::log::alevel::all );
                _client.set_message_handler( [this]( connection_hdl hdl,
                                                  typename websocketpp::client<T>::message_ptr msg ){
                   fc::async( [this,&msg](){
                        wdump((msg->get_payload()));
                        fc::async( [this, received = msg->get_payload()](){
                           if( _connection )
                              _connection->on_message(received);
                        });
                   }, _client_thread_id ).wait();
                });
                _client.set_close_handler( [this] ( connection_hdl hdl ) {
                   fc::async( [this](){
                      std::unique_lock<boost::fibers::mutex> lock(_mtx);
                      if( _connection ) {
                         _connection->closed();
                         _connection.reset();
                      }
                      _connected = false;
                      _error.reset();
                      cv.notify_all();
                   }, _client_thread_id ).wait();
                });
                _client.set_fail_handler( [this]( connection_hdl hdl ){
                   auto con = _client.get_con_from_hdl(hdl);
                   auto message = con->get_ec().message();
                   if( _connection )
                      fc::async( [this] () {
                         std::unique_lock<boost::fibers::mutex> lock(_mtx);
                         if( _connection )
                            _connection->closed();
                         _connection.reset();
                      }, _client_thread_id ).wait();
                   std::unique_lock<boost::fibers::mutex> lock(_mtx);
                   _error = std::make_shared<exception>( FC_LOG_MESSAGE( error, "${message}",
                                                                         ("message",message) ) );
                   _connected = false;
                   cv.notify_all();
                });

                _client.init_asio( &fc::asio::default_io_service() );
            }
            virtual ~generic_websocket_client_impl()
            {
               std::unique_lock<boost::fibers::mutex> lock(_mtx);
               if( _connection )
               {
                  _connection->close(0, "client closed");
                  _connection.reset();
               }
               cv.wait( lock, [this] () { return !_connected; } );
            }

            template<typename C>
            websocket_connection_ptr connect( const std::string& uri )
            {
               std::unique_lock<boost::fibers::mutex> lock(_mtx);
               FC_ASSERT( !_connected, "Already connected!" );

               _error.reset();
               websocketpp::lib::error_code ec;
               _uri = uri;
               _client.set_open_handler( [this]( websocketpp::connection_hdl hdl ){
                  _hdl = hdl;
                  auto con = _client.get_con_from_hdl(hdl);
                  _connection = std::make_shared<detail::websocket_connection_impl<C>>( con );
                  std::unique_lock<boost::fibers::mutex> lock(_mtx);
                  _connected = true;
                  cv.notify_all();
               });

               auto con = _client.get_connection( uri, ec );

               if( ec ) FC_ASSERT( !ec, "error: ${e}", ("e",ec.message()) );

               _client.connect(con);
               cv.wait( lock, [this] () { return _connected || _error; } );
               if( _error ) throw *_error;
               return _connection;
            }
            std::thread::id                    _client_thread_id;
            websocketpp::client<T>             _client;
            websocket_connection_ptr           _connection;
            std::string                        _uri;
            fc::optional<connection_hdl>       _hdl;

            boost::fibers::mutex _mtx;
            boost::fibers::condition_variable cv;
            bool _connected = false;
            exception_ptr _error;
      };

      class websocket_client_impl : public generic_websocket_client_impl<asio_with_stub_log>
      {};

      class websocket_tls_client_impl : public generic_websocket_client_impl<asio_tls_stub_log>
      {
         public:
            websocket_tls_client_impl( const std::string& ca_filename )
            : generic_websocket_client_impl()
            {
                // ca_filename has special values:
                // "_none" disables cert checking (potentially insecure!)
                // "_default" uses default CA's provided by OS

                _client.set_tls_init_handler( [this, ca_filename](websocketpp::connection_hdl) {
                   context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
                   try {
                      ctx->set_options(boost::asio::ssl::context::default_workarounds |
                      boost::asio::ssl::context::no_sslv2 |
                      boost::asio::ssl::context::no_sslv3 |
                      boost::asio::ssl::context::single_dh_use);

                      setup_peer_verify( ctx, ca_filename );
                   } catch (std::exception& e) {
                      edump((e.what()));
                      std::cout << e.what() << std::endl;
                   }
                   return ctx;
                });

            }
            virtual ~websocket_tls_client_impl() {}

            std::string get_host()const
            {
               return websocketpp::uri( _uri ).get_host();
            }

            void setup_peer_verify( context_ptr& ctx, const std::string& ca_filename )
            {
               if( ca_filename == "_none" )
                  return;
               ctx->set_verify_mode( boost::asio::ssl::verify_peer );
               if( ca_filename == "_default" )
               {
#if WIN32
                  add_windows_root_certs( *ctx );
#else
                  ctx->set_default_verify_paths();
#endif
               }
               else
                  ctx->load_verify_file( ca_filename );
               ctx->set_verify_depth(10);
               ctx->set_verify_callback( boost::asio::ssl::rfc2818_verification( get_host() ) );
            }

      };


   } // namespace detail

   websocket_server::websocket_server():my( new detail::websocket_server_impl() ) {}
   websocket_server::~websocket_server(){}

   void websocket_server::on_connection( const on_connection_handler& handler )
   {
      my->_on_connection = handler;
   }

   void websocket_server::listen( uint16_t port )
   {
      my->_server.listen(port);
   }
   void websocket_server::listen( const fc::ip::endpoint& ep )
   {
       my->_server.listen( boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4(uint32_t(ep.get_address())),ep.port()) );
   }

   uint16_t websocket_server::get_listening_port()
   {
       websocketpp::lib::asio::error_code ec;
       return my->_server.get_local_endpoint(ec).port();
   }

   void websocket_server::start_accept() {
       my->_server.start_accept();
   }

   void websocket_server::stop_listening()
   {
       my->_server.stop_listening();
   }

   void websocket_server::close()
   {
       for (auto& connection : my->_connections)
           my->_server.close(connection.first, websocketpp::close::status::normal, "Goodbye");
   }



   websocket_tls_server::websocket_tls_server( const string& server_pem, const string& ssl_password ):my( new detail::websocket_tls_server_impl(server_pem, ssl_password) ) {}
   websocket_tls_server::~websocket_tls_server(){}

   void websocket_tls_server::on_connection( const on_connection_handler& handler )
   {
      my->_on_connection = handler;
   }

   void websocket_tls_server::listen( uint16_t port )
   {
      my->_server.listen(port);
   }
   void websocket_tls_server::listen( const fc::ip::endpoint& ep )
   {
      my->_server.listen( boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4(uint32_t(ep.get_address())),ep.port()) );
   }

   void websocket_tls_server::start_accept() {
      my->_server.start_accept();
   }

   websocket_client::websocket_client( const std::string& ca_filename ):my( new detail::websocket_client_impl() ),smy(new detail::websocket_tls_client_impl( ca_filename )) {}
   websocket_client::~websocket_client(){ }

   websocket_connection_ptr websocket_client::connect( const std::string& uri )
   { try {
       if( uri.substr(0,4) == "wss:" )
          return secure_connect(uri);
       FC_ASSERT( uri.substr(0,3) == "ws:" );

       return my->connect<detail::websocket_client_connection_type>( uri );
   } FC_CAPTURE_AND_RETHROW( (uri) ) }

   websocket_connection_ptr websocket_client::secure_connect( const std::string& uri )
   { try {
       if( uri.substr(0,3) == "ws:" )
          return connect(uri);
       FC_ASSERT( uri.substr(0,4) == "wss:" );

       return smy->connect<detail::websocket_tls_client_connection_type>( uri );
   } FC_CAPTURE_AND_RETHROW( (uri) ) }

   void websocket_client::close()
   {
       if (my->_hdl)
           my->_client.close(*my->_hdl, websocketpp::close::status::normal, "Goodbye");
   }

   void websocket_client::synchronous_close()
   {
       close();
       std::unique_lock<boost::fibers::mutex> lock(my->_mtx);
       my->cv.wait( lock, [this] () { return !my->_connected; } );
   }

} } // fc::http
