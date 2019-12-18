#include <boost/test/unit_test.hpp>

#include <boost/fiber/operations.hpp>

#include <fc/network/http/websocket.hpp>

#include <chrono>
#include <iostream>

#include "../../thread/worker_thread.hxx"

using namespace fc::test;

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(websocket_test)
{ 
    fc::http::websocket_client client;
    fc::http::websocket_connection_ptr s_conn, c_conn;
    fc::initialize_fibers();
    int port;
    sync_point client_sync;
    {
        sync_point server_sync;
        fc::http::websocket_server server;
        server.on_connection([&s_conn,&server_sync]( const fc::http::websocket_connection_ptr& c ){
                s_conn = c;
                c->on_message_handler([c](const std::string& s){
                    c->send_message("echo: " + s);
                });
                server_sync.set();
            });

        server.listen( 0 );
        port = server.get_listening_port();

        server.start_accept();

        std::string echo;
        c_conn = client.connect( "ws://localhost:" + fc::to_string(port) );
        server_sync.wait();
        c_conn->on_message_handler([&echo,&client_sync](const std::string& s){
           echo = s;
           client_sync.set();
        });
        c_conn->send_message( "hello world" );
        client_sync.wait();
        BOOST_CHECK_EQUAL("echo: hello world", echo);

        client_sync.reset();
        c_conn->send_message( "again" );
        client_sync.wait();
        BOOST_CHECK_EQUAL("echo: again", echo);

        client_sync.reset();
        c_conn->closed.connect([&client_sync] () {
           client_sync.set();
        });
        s_conn->close(0, "test");
        client_sync.wait();
        BOOST_CHECK_THROW(c_conn->send_message( "again" ), fc::exception);

        server_sync.reset();
        c_conn = client.connect( "ws://localhost:" + fc::to_string(port) );
        server_sync.wait();
        c_conn->on_message_handler([&echo,&client_sync](const std::string& s){
           echo = s;
           client_sync.set();
        });
        client_sync.reset();
        c_conn->send_message( "hello world" );
        client_sync.wait();
        BOOST_CHECK_EQUAL("echo: hello world", echo);

        client_sync.reset();
        c_conn->closed.connect([&client_sync] () {
           client_sync.set();
        });
    }
    client_sync.wait();

    BOOST_CHECK_THROW(c_conn->send_message( "again" ), fc::assert_exception);
    BOOST_CHECK_THROW(client.connect( "ws://localhost:" + fc::to_string(port) ), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
