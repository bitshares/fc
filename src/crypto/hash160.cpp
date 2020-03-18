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

#include <fc/crypto/hex.hpp>
#include <fc/fwd_impl.hpp>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <string.h>
#include <fc/crypto/hash160.hpp>
#include <fc/variant.hpp>
#include <vector>
#include "_digest_common.hpp"

namespace fc 
{
  
hash160::hash160() { memset( _hash, 0, sizeof(_hash) ); }

hash160::hash160( const string& hex_str ) {
   fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );  
}

string hash160::str()const {
   return fc::to_hex( (char*)_hash, sizeof(_hash) );
}

hash160::operator string()const { return  str(); }

char* hash160::data()const { return (char*)&_hash[0]; }

class hash160::encoder::impl {
   public:
   impl() { }
};

hash160::encoder::~encoder() {}
hash160::encoder::encoder() {}

hash160 hash160::hash( const char* d, uint32_t dlen ) {
   encoder e;
   e.write(d,dlen);
   return e.result();
}

hash160 hash160::hash( const string& s ) {
   return hash( s.c_str(), s.size() );
}

void hash160::encoder::write( const char* d, uint32_t dlen )
{
   for(uint32_t i = 0; i < dlen; ++i)
      bytes.push_back(d[i]); 
}

hash160 hash160::encoder::result() {
   // perform the first hashing function
   SHA256_CTX sha_ctx;
   SHA256_Init(&sha_ctx);
   SHA256_Update( &sha_ctx, bytes.data(), bytes.size());
   unsigned char sha_hash[SHA256_DIGEST_LENGTH];
   SHA256_Final( sha_hash, &sha_ctx );
   // perform the second hashing function
   RIPEMD160_CTX ripe_ctx;
   RIPEMD160_Init(&ripe_ctx);
   RIPEMD160_Update( &ripe_ctx, sha_hash, SHA256_DIGEST_LENGTH );
   hash160 h;
   RIPEMD160_Final( (uint8_t *)h.data(), &ripe_ctx );
   return h;
}

hash160 operator << ( const hash160& h1, uint32_t i ) {
   hash160 result;
   fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
   return result;
}

hash160 operator ^ ( const hash160& h1, const hash160& h2 ) {
   hash160 result;
   result._hash[0] = h1._hash[0].value() ^ h2._hash[0].value();
   result._hash[1] = h1._hash[1].value() ^ h2._hash[1].value();
   result._hash[2] = h1._hash[2].value() ^ h2._hash[2].value();
   result._hash[3] = h1._hash[3].value() ^ h2._hash[3].value();
   result._hash[4] = h1._hash[4].value() ^ h2._hash[4].value();
   return result;
}

bool operator >= ( const hash160& h1, const hash160& h2 ) {
   return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) >= 0;
}

bool operator > ( const hash160& h1, const hash160& h2 ) {
   return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) > 0;
}

bool operator < ( const hash160& h1, const hash160& h2 ) {
   return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) < 0;
}

bool operator != ( const hash160& h1, const hash160& h2 ) {
   return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) != 0;
}

bool operator == ( const hash160& h1, const hash160& h2 ) {
   return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) == 0;
}
  
void to_variant( const hash160& bi, variant& v, uint32_t max_depth )
{
   to_variant( std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) ), v, max_depth );
}

void from_variant( const variant& v, hash160& bi, uint32_t max_depth )
{
   std::vector<char> ve = v.as< std::vector<char> >( max_depth );
   memset( &bi, char(0), sizeof(bi) );
   if( ve.size() )
      memcpy( &bi, ve.data(), std::min<size_t>(ve.size(),sizeof(bi)) );
}
  
} // fc
