#include <fc/http/connection.hpp>
#include <fc/tcp_socket.hpp>
#include <fc/sstream.hpp>
#include <fc/iostream.hpp>
#include <fc/exception.hpp>
#include <fc/ip.hpp>
#include <fc/error_report.hpp>
#include <fc/hex.hpp>


FC_START_SHARED_IMPL(fc::http::connection)
   fc::tcp_socket sock;
   fc::ip::endpoint ep;
   impl() {
   }

   int read_until( char* buffer, char* end, char c = '\n' ) {
      char* p = buffer;
     // try {
          while( p < end && !sock.read(p,1).eof() ) {
            if( *p == c ) {
              *p = '\0';
              return (p - buffer)-1;
            }
            ++p;
          }
     // } catch ( ... ) {
     //   elog("%s", fc::current_exception().diagnostic_information().c_str() );
        //elog( "%s", fc::except_str().c_str() );
     // }
      return (p-buffer);
   }

   fc::http::reply parse_reply() {
      fc::http::reply rep;
      try {
        fc::vector<char> line(1024*8);
        int s = read_until( line.data(), line.data()+line.size(), ' ' ); // HTTP/1.1
        s = read_until( line.data(), line.data()+line.size(), ' ' ); // CODE
        rep.status = fc::lexical_cast<int>(fc::string(line.data()));
        s = read_until( line.data(), line.data()+line.size(), '\n' ); // DESCRIPTION
        
        while( (s = read_until( line.data(), line.data()+line.size(), '\n' )) > 1 ) {
          fc::http::header h;
          char* end = line.data();
          while( *end != ':' )++end;
          h.key = fc::string(line.data(),end);
          ++end; // skip ':'
          ++end; // skip space
          char* skey = end;
          while( *end != '\r' ) ++end;
          h.val = fc::string(skey,end);
          rep.headers.push_back(h);
          if( h.key == "Content-Length" ) {
             rep.body.resize( fc::lexical_cast<int>( fc::string(h.val) ) );
          }
        }
        if( rep.body.size() ) {
          //slog( "Reading body size %d", rep.body.size() );
          sock.read( rep.body.data(), rep.body.size() );
        }
        return rep;
      } catch ( ... ) {
        elog( "%s", fc::except_str().c_str() );
        sock.close();
        FC_THROW_REPORT( "Error parsing reply" );
        rep.status = http::reply::InternalServerError;
        return rep;
      } 
   }

FC_END_SHARED_IMPL 
#include <fc/shared_impl.cpp>



namespace fc { namespace http {

FC_REFERENCE_TYPE_IMPL( connection )


// used for clients
void       connection::connect_to( const fc::ip::endpoint& ep ) {
  my->sock.close();
  my->sock.connect_to( my->ep = ep );
}

http::reply connection::request( const fc::string& method, 
                                const fc::string& url, 
                                const fc::string& body ) {
	
  if( !my->sock.is_open() ) {
    wlog( "Re-open socket!" );
    my->sock.connect_to( my->ep );
  }
  try {
      fc::stringstream req;
      req << method <<" "<<url<<" HTTP/1.1\r\n";
      req << "Host: localhost\r\n";
      req << "Content-Type: application/json\r\n";
      if( body.size() ) req << "Content-Length: "<< body.size() << "\r\n";
      req << "\r\n"; 
      fc::string head = req.str();

      my->sock.write( head.c_str(), head.size() );
    //  fc::cerr.write( head.c_str() );

      if( body.size() )  {
          my->sock.write( body.c_str(), body.size() );
    //      fc::cerr.write( body.c_str() );
      }
    //  fc::cerr.flush();

      return my->parse_reply();
  } catch ( ... ) {
      my->sock.close();
      FC_THROW_REPORT( "Error Sending HTTP Request" ); // TODO: provide more info
   //  return http::reply( http::reply::InternalServerError ); // TODO: replace with connection error
  }
}

// used for servers
fc::tcp_socket& connection::get_socket()const {
  return my->sock;
}

http::request    connection::read_request()const {
  http::request req;
  fc::vector<char> line(1024*8);
  int s = my->read_until( line.data(), line.data()+line.size(), ' ' ); // METHOD
  req.method = line.data();
  s = my->read_until( line.data(), line.data()+line.size(), ' ' ); // PATH
  req.path = line.data();
  s = my->read_until( line.data(), line.data()+line.size(), '\n' ); // HTTP/1.0
  
  while( (s = my->read_until( line.data(), line.data()+line.size(), '\n' )) > 1 ) {
    fc::http::header h;
    char* end = line.data();
    while( *end != ':' )++end;
    h.key = fc::string(line.data(),end);
    ++end; // skip ':'
    ++end; // skip space
    char* skey = end;
    while( *end != '\r' ) ++end;
    h.val = fc::string(skey,end);
    req.headers.push_back(h);
    if( h.key == "Content-Length" ) {
       req.body.resize( fc::lexical_cast<int>( fc::string(h.val) ) );
    }
    if( h.key == "Host" ) {
       req.domain = h.val;
    }
  }
  if( req.body.size() ) {
    slog( "Reading body size %d", req.body.size() );
    my->sock.read( req.body.data(), req.body.size() );
  }
  return req;
}

fc::string request::get_header( const fc::string& key )const {
  for( auto itr = headers.begin(); itr != headers.end(); ++itr ) {
    if( itr->key == key ) { return itr->val; } 
  }
  return fc::string();
}
fc::vector<header> parse_urlencoded_params( const fc::string& f ) {
  int num_args = 0;
  for( size_t i = 0; i < f.size(); ++i ) {
    if( f[i] == '=' ) ++num_args;
  }
  fc::vector<header> h(num_args);
  int arg = 0;
  for( size_t i = 0; i < f.size(); ++i ) {
    while( f[i] != '=' && i < f.size() ) {
      if( f[i] == '%' ) { 
        h[arg].key += char((fc::from_hex(f[i+1]) << 4) | fc::from_hex(f[i+2]));
        i += 3;
      } else {
          h[arg].key += f[i];
          ++i;
      }
    }
    ++i;
    while( i < f.size() && f[i] != '&' ) {
      if( f[i] == '%' ) { 
        h[arg].val += char((fc::from_hex(f[i+1]) << 4) | fc::from_hex(f[i+2]));
        i += 3;
      } else {
        h[arg].val += f[i] == '+' ? ' ' : f[i];
        ++i;
      }
    }
    ++arg;
  }
  return h;
}

} } // fc::http