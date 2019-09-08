#pragma once
#include <boost/endian/buffers.hpp>
#include <fc/fwd.hpp>
#include <fc/io/raw_fwd.hpp>

#include <functional>
#include <string>

namespace fc{

class sha1 
{
  public:
    sha1();
    explicit sha1( const std::string& hex_str );

    std::string str()const;
    operator    std::string()const;

    char*    data()const;
    static constexpr size_t data_size() { return 20; }

    static sha1 hash( const char* d, uint32_t dlen );
    static sha1 hash( const std::string& );

    template<typename T>
    static sha1 hash( const T& t ) 
    {
      sha1::encoder e; 
      fc::raw::pack(e, t); 
      return e.result(); 
    }

    class encoder 
    {
      public:
        encoder();
        ~encoder();

        void write( const char* d, uint32_t dlen );
        void put( char c ) { write( &c, 1 ); }
        void reset();
        sha1 result();

      private:
        struct      impl;
        fc::fwd<impl,96> my;
    };

    template<typename T>
    inline friend T& operator<<( T& ds, const sha1& ep ) {
      ds.write( ep.data(), sizeof(ep) );
      return ds;
    }

    template<typename T>
    inline friend T& operator>>( T& ds, sha1& ep ) {
      ds.read( ep.data(), sizeof(ep) );
      return ds;
    }
    friend sha1 operator << ( const sha1& h1, uint32_t i       );
    friend bool   operator == ( const sha1& h1, const sha1& h2 );
    friend bool   operator != ( const sha1& h1, const sha1& h2 );
    friend sha1 operator ^  ( const sha1& h1, const sha1& h2 );
    friend bool   operator >= ( const sha1& h1, const sha1& h2 );
    friend bool   operator >  ( const sha1& h1, const sha1& h2 ); 
    friend bool   operator <  ( const sha1& h1, const sha1& h2 ); 
                             
    boost::endian::little_uint32_buf_t _hash[5];
};

namespace raw {

   template<typename T>
   inline void pack( T& ds, const sha1& ep, uint32_t _max_depth ) {
      ds << ep;
   }

   template<typename T>
   inline void unpack( T& ds, sha1& ep, uint32_t _max_depth ) {
      ds >> ep;
   }

}

  class variant;
  void to_variant( const sha1& bi, variant& v, uint32_t max_depth );
  void from_variant( const variant& v, sha1& bi, uint32_t max_depth );

} // namespace fc

namespace std
{
    template<>
    struct hash<fc::sha1>
    {
       size_t operator()( const fc::sha1& s )const
       {
           return  *((size_t*)&s);
       }
    };
}

#include <fc/reflect/reflect.hpp>
FC_REFLECT_TYPENAME( fc::sha1 )
