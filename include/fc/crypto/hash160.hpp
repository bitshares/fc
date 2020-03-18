/*
 * Copyright (c) 2018 jmjatlanta and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <boost/endian/buffers.hpp>
#include <fc/fwd.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/reflect/typename.hpp>

namespace fc{

class hash160
{
   public:
   hash160();
   explicit hash160( const string& hex_str );

   string str()const;
   explicit operator string()const;

   char* data() const;
   size_t data_size() const { return 160/8; }

   static hash160 hash( const char* d, uint32_t dlen );
   static hash160 hash( const string& );

   template<typename T>
   static hash160 hash( const T& t ) 
   { 
      hash160::encoder e; 
      fc::raw::pack(e,t);
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
      hash160 result();

      private:
      class impl;
      fc::fwd<impl,96> my;
      std::vector<uint8_t> bytes;
   };

   template<typename T>
   inline friend T& operator<<( T& ds, const hash160& ep ) {
      ds.write( ep.data(), sizeof(ep) );
      return ds;
   }

   template<typename T>
   inline friend T& operator>>( T& ds, hash160& ep ) {
      ds.read( ep.data(), sizeof(ep) );
      return ds;
   }
   friend hash160 operator << ( const hash160& h1, uint32_t i       );
   friend bool operator == ( const hash160& h1, const hash160& h2 );
   friend bool operator != ( const hash160& h1, const hash160& h2 );
   friend hash160 operator ^  ( const hash160& h1, const hash160& h2 );
   friend bool operator >= ( const hash160& h1, const hash160& h2 );
   friend bool operator >  ( const hash160& h1, const hash160& h2 );
   friend bool operator <  ( const hash160& h1, const hash160& h2 );

   boost::endian::little_uint32_buf_t _hash[5];
};

namespace raw {

   template<typename T>
   inline void pack( T& ds, const hash160& ep, uint32_t _max_depth ) {
      ds << ep;
   }

   template<typename T>
   inline void unpack( T& ds, hash160& ep, uint32_t _max_depth ) {
      ds >> ep;
   }

}

   class variant;
   void to_variant( const hash160& bi, variant& v, uint32_t max_depth );
   void from_variant( const variant& v, hash160& bi, uint32_t max_depth );

  template<> struct get_typename<hash160>    { static const char* name()  { return "hash160";  } };
} // namespace fc

namespace std
{
   template<>
   struct hash<fc::hash160>
   {
      size_t operator()( const fc::hash160& s )const
      {
         return  *((size_t*)&s);
      }
   };
}
