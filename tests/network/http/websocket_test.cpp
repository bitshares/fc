#include <boost/test/unit_test.hpp>

#include <fc/network/http/websocket.hpp>

#include <fc/log/logger.hpp>
#include <fc/log/console_appender.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(websocket_test)
{ 
   // set up logging
   fc::shared_ptr<fc::console_appender> ca(new fc::console_appender);
   fc::logger l = fc::logger::get("rpc");
   l.add_appender(ca);

    fc::http::websocket_client client;
    fc::http::websocket_connection_ptr s_conn, c_conn;
    int port;
    {
        fc::http::websocket_server server;
        server.on_connection([&]( const fc::http::websocket_connection_ptr& c ){
                s_conn = c;
                c->on_message_handler([&](const std::string& s){
                    c->send_message("echo: " + s);
                });
            });

        bool listen_ok = false;
        for( int i = 0; !listen_ok && i < 5; ++i )
        {
           port = std::rand() % 50000 + 10000;
           try
           {
              server.listen( port );
              listen_ok = true;
           }
           catch( const std::exception& )
           {
              // if the port is busy, listen() will throw a std::exception, do nothing here.
           }
        }
        BOOST_REQUIRE( listen_ok );

        server.start_accept();

        std::string echo;
        c_conn = client.connect( "ws://localhost:" + fc::to_string(port) );
        c_conn->on_message_handler([&](const std::string& s){
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
        c_conn->send_message( "again" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: again", echo);

        s_conn->close(0, "test");
        fc::usleep( fc::seconds(1) );
        try {
            c_conn->send_message( "again" );
            BOOST_FAIL("expected assertion failure");
        } catch (const fc::exception& e) {
            //std::cerr << e.to_string() << "\n";
        }

        c_conn = client.connect( "ws://localhost:" + fc::to_string(port) );
        c_conn->on_message_handler([&](const std::string& s){
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
    }

    try {
        c_conn->send_message( "again" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::assert_exception& e) {
        std::cerr << "Expected assertion failure : " << e.to_string() << "\n";
    } catch (const fc::exception& e) {
        BOOST_FAIL("Unexpected exception : " + e.to_string());
    } catch (const std::exception& e) {
        BOOST_FAIL("Unexpected exception : " + std::string(e.what()));
    }

    try {
        c_conn = client.connect( "ws://localhost:" + fc::to_string(port) );
        BOOST_FAIL("expected fc::exception (fail to connect)");
    } catch (const fc::exception& e) {
        std::cerr << "Excepted fc::exception : " << e.to_string() << "\n";
    } catch (const std::exception& e) {
        BOOST_FAIL("Unexpected exception : " + std::string(e.what()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
