#pragma once
#include <memory>
#include <string>
#include <vector>

namespace fc { 
  namespace ip { class endpoint; }
  class tcp_socket;

  namespace http {

     struct header 
     {
       header( std::string k, std::string v )
       :key(std::move(k)),val(std::move(v)){}
       header(){}
       std::string key;
       std::string val;
     };

     typedef std::vector<header> headers;
     
     struct reply 
     {
        enum status_code {
            OK                  = 200,
            RecordCreated       = 201,
            BadRequest          = 400,
            NotAuthorized       = 401,
            NotFound            = 404,
            Found               = 302,
            InternalServerError = 500
        };
        reply( status_code c = OK):status(c){}
        int                     status;
        std::vector<header>      headers;
        std::vector<char>        body;
     };
     
     struct request 
     {
        std::string get_header( const std::string& key )const;
        std::string              remote_endpoint;
        std::string              method;
        std::string              domain;
        std::string              path;
        std::vector<header>     headers;
        std::vector<char>       body;
     };
     
     std::vector<header> parse_urlencoded_params( const std::string& f );
     
     /**
      *  Connections have reference semantics, all copies refer to the same
      *  underlying socket.  
      */
     class connection 
     {
       public:
         connection();
         ~connection();
         // used for clients
         void         connect_to( const fc::ip::endpoint& ep );
         http::reply  request( const std::string& method, const std::string& url, const std::string& body = std::string(), const headers& = headers());
     
         // used for servers
         fc::tcp_socket& get_socket()const;
     
         http::request    read_request()const;

         class impl;
       private:
         std::unique_ptr<impl> my;
     };
     
     typedef std::shared_ptr<connection> connection_ptr;

} } // fc::http

#include <fc/reflect/reflect.hpp>
FC_REFLECT( fc::http::header, (key)(val) )

