#include <fc/sstream.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/log.hpp>
#include <sstream>

namespace fc {
  class stringstream::impl {
    public:
    impl( fc::string&s )
    :ss( s )
    { }
    impl( const fc::string&s )
    :ss( s )
    { }
    impl(){}
    
    std::stringstream ss;
  };

  stringstream::stringstream( fc::string& s )
  :my(s) {
  }
  stringstream::stringstream( const fc::string& s )
  :my(s) {
  }
  stringstream::stringstream(){}
  stringstream::~stringstream(){}


  fc::string stringstream::str(){
    return my->ss.str();//.c_str();//*reinterpret_cast<fc::string*>(&st);
  }

  bool     stringstream::eof()const {
    return my->ss.eof();
  }
  ostream& stringstream::write( const char* buf, size_t len ) {
    my->ss.write(buf,len);
    return *this;
  }
  size_t   stringstream::readsome( char* buf, size_t len ) {
    return static_cast<size_t>(my->ss.readsome(buf,len));
  }
  istream&   stringstream::read( char* buf, size_t len ) {
    my->ss.read(buf,len);
    return *this;
  }
  void     stringstream::close(){ my->ss.flush(); };
  void     stringstream::flush(){ my->ss.flush(); };

  istream& stringstream::read( int64_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint64_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int32_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint32_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int16_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint16_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int8_t&      v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint8_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( float&       v ) { my->ss >> v; return *this; }
  istream& stringstream::read( double&      v ) { my->ss >> v; return *this; }
  istream& stringstream::read( bool&        v ) { my->ss >> v; return *this; }
  istream& stringstream::read( char&        v ) { my->ss >> v; return *this; }
  istream& stringstream::read( fc::string&  v ) { my->ss >> *reinterpret_cast<std::string*>(&v); return *this; }

  ostream& stringstream::write( const fc::string& s) {
    my->ss.write( s.c_str(), s.size() );
    return *this;
  }

} 

