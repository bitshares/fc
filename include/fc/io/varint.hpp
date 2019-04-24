#pragma once
#include <stdint.h>

namespace fc {

struct unsigned_int {
    unsigned_int( uint64_t v = 0 ):value(v){}

    template<typename T>
    unsigned_int( T v ):value(v){}

    template<typename T>
    operator T()const { return static_cast<T>(value); }

    unsigned_int& operator=( uint64_t v ) { value = v; return *this; }
    
    uint64_t value;

    friend bool operator==( const unsigned_int& i, const uint64_t& v )     { return i.value == v; }
    friend bool operator==( const uint64_t& i, const unsigned_int& v )     { return i       == v.value; }
    friend bool operator==( const unsigned_int& i, const unsigned_int& v ) { return i.value == v.value; }

    friend bool operator!=( const unsigned_int& i, const uint64_t& v )     { return i.value != v; }
    friend bool operator!=( const uint64_t& i, const unsigned_int& v )     { return i       != v.value; }
    friend bool operator!=( const unsigned_int& i, const unsigned_int& v ) { return i.value != v.value; }

    friend bool operator<( const unsigned_int& i, const uint64_t& v )      { return i.value < v; }
    friend bool operator<( const uint64_t& i, const unsigned_int& v )      { return i       < v.value; }
    friend bool operator<( const unsigned_int& i, const unsigned_int& v )  { return i.value < v.value; }

    friend bool operator<=( const unsigned_int& i, const uint64_t& v )     { return i.value <= v; }
    friend bool operator<=( const uint64_t& i, const unsigned_int& v )     { return i       <= v.value; }
    friend bool operator<=( const unsigned_int& i, const unsigned_int& v ) { return i.value <= v.value; }

    friend bool operator>( const unsigned_int& i, const uint64_t& v )      { return i.value > v; }
    friend bool operator>( const uint64_t& i, const unsigned_int& v )      { return i       > v.value; }
    friend bool operator>( const unsigned_int& i, const unsigned_int& v )  { return i.value > v.value; }

    friend bool operator>=( const unsigned_int& i, const uint64_t& v )     { return i.value >= v; }
    friend bool operator>=( const uint64_t& i, const unsigned_int& v )     { return i       >= v.value; }
    friend bool operator>=( const unsigned_int& i, const unsigned_int& v ) { return i.value >= v.value; }
};

class variant;

void to_variant( const unsigned_int& var, variant& vo, uint32_t max_depth = 1 );
void from_variant( const variant& var, unsigned_int& vo, uint32_t max_depth = 1 );

}  // namespace fc

#include <unordered_map>
namespace std
{
   template<>
   struct hash<fc::unsigned_int>
   {
       public:
         size_t operator()(const fc::unsigned_int &a) const
         {
            return std::hash<uint64_t>()(a.value);
         }
   };
}
