#include <boost/lexical_cast.hpp>

#include <fc/io/iostream.hpp>
#include <fc/io/sstream.hpp>

#include <string>

namespace fc {

  fc::istream& getline( fc::istream& i, std::string& s, char delim  ) {
    fc::stringstream ss; 
    char c;
    i.read( &c, 1 );
    while( true ) {
      if( c == delim ) { s = ss.str();  return i; }
      if( c != '\r' ) ss.write(&c,1);
      i.read( &c, 1 );
    }
    s = ss.str();
    return i;
  }

  ostream& operator<<( ostream& o, const char v )
  {
     o.write( &v, 1 );
     return o;
  }
  ostream& operator<<( ostream& o, const char* v )
  {
     o.write( v, strlen(v) );
     return o;
  }

  ostream& operator<<( ostream& o, const std::string& v )
  {
     o.write( v.c_str(), v.size() );
     return o;
  }

  ostream& operator<<( ostream& o, const double& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const float& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int64_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint64_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int32_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint32_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int16_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint16_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int8_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint8_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

#ifdef __APPLE__
  ostream& operator<<( ostream& o, const size_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

#endif

  istream& operator>>( istream& o, std::string& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, char& v )
  {
     o.read(&v,1);
     return o;
  }

  char istream::get()
  {
    char tmp;
    read(&tmp,1);
    return tmp;
  }

  istream& istream::read( char* buf, size_t len )
  {
      char* pos = buf;
      while( size_t(pos-buf) < len )
         pos += readsome( pos, len - (pos - buf) );
      return *this;
  }

  istream& istream::read( const std::shared_ptr<char>& buf, size_t len, size_t offset )
  {
    size_t bytes_read = 0;
    while( bytes_read < len )
      bytes_read += readsome(buf, len - bytes_read, bytes_read + offset);
    return *this;
  }

  ostream& ostream::write( const char* buf, size_t len )
  {
      const char* pos = buf;
      while( size_t(pos-buf) < len )
         pos += writesome( pos, len - (pos - buf) );
      return *this;
  }

  ostream& ostream::write( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
  {
    size_t bytes_written = 0;
    while( bytes_written < len )
      bytes_written += writesome(buf, len - bytes_written, bytes_written + offset);
    return *this;
  }

} // namespace fc
