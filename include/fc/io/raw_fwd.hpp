#pragma once
#include <fc/config.hpp>
#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/array.hpp>
#include <fc/safe.hpp>
#include <deque>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <set>

#define MAX_ARRAY_ALLOC_SIZE (1024*1024*10)

namespace fc {
   class time_point;
   class time_point_sec;
   class variant;
   class variant_object;
   class path;
   template<typename... Types> class static_variant;

   class sha224;
   class sha256;
   class sha512;
   class ripemd160;

   template<typename IntType, typename EnumType> class enum_type;
   namespace ip { class endpoint; }

   namespace ecc { class public_key; class private_key; }

   namespace raw {
    template<typename T>
    inline size_t pack_size(  const T& v );

    template<typename Stream, typename IntType, typename EnumType>
    inline void pack( Stream& s, const fc::enum_type<IntType,EnumType>& tp, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename IntType, typename EnumType>
    inline void unpack( Stream& s, fc::enum_type<IntType,EnumType>& tp, uint32_t _max_depth=FC_PACK_MAX_DEPTH );



    template<typename Stream, typename T> inline void pack( Stream& s, const std::set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, std::set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void pack( Stream& s, const std::unordered_set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, std::unordered_set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename... T> void pack( Stream& s, const static_variant<T...>& sv, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename... T> void unpack( Stream& s, static_variant<T...>& sv, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    //template<typename Stream, typename T> inline void pack( Stream& s, const flat_set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    //template<typename Stream, typename T> inline void unpack( Stream& s, flat_set<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename T> inline void pack( Stream& s, const std::deque<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, std::deque<T>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename K, typename V> inline void pack( Stream& s, const std::unordered_map<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename K, typename V> inline void unpack( Stream& s, std::unordered_map<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename K, typename V> inline void pack( Stream& s, const std::map<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename K, typename V> inline void unpack( Stream& s, std::map<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    //template<typename Stream, typename K, typename... V> inline void pack( Stream& s, const flat_map<K,V...>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    //template<typename Stream, typename K, typename V, typename... A> inline void unpack( Stream& s, flat_map<K,V,A...>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename K, typename V> inline void pack( Stream& s, const std::pair<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename K, typename V> inline void unpack( Stream& s, std::pair<K,V>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void pack( Stream& s, const variant_object& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, variant_object& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const variant& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, variant& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void pack( Stream& s, const path& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, path& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const ip::endpoint& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, ip::endpoint& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );


    template<typename Stream, typename T> void unpack( Stream& s, fc::optional<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> void unpack( Stream& s, const T& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> void pack( Stream& s, const fc::optional<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> void pack( Stream& s, const safe<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> void unpack( Stream& s, fc::safe<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> void unpack( Stream& s, time_point&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void pack( Stream& s, const time_point&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void unpack( Stream& s, time_point_sec&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void pack( Stream& s, const time_point_sec&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void unpack( Stream& s, std::string&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void pack( Stream& s, const std::string&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void unpack( Stream& s, fc::ecc::public_key&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void pack( Stream& s, const fc::ecc::public_key&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void unpack( Stream& s, fc::ecc::private_key&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> void pack( Stream& s, const fc::ecc::private_key&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void unpack( Stream& s, fc::sha224&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const fc::sha224&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, fc::sha256&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const fc::sha256&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, fc::sha512&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const fc::sha512&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, fc::ripemd160&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const fc::ripemd160&, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename T> inline void pack( Stream& s, const T& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, T& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename T> inline void pack( Stream& s, const std::vector<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, std::vector<T>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void pack( Stream& s, const unsigned_int& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void pack( Stream& s, const char* v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void pack( Stream& s, const std::vector<char>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, std::vector<char>& value, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename T, size_t N> inline void pack( Stream& s, const fc::array<T,N>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T, size_t N> inline void unpack( Stream& s, fc::array<T,N>& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH);

    template<typename Stream, typename T> inline void pack( Stream& s, const shared_ptr<T>& v,
                                                            uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, shared_ptr<T>& v,
                                                              uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream, typename T> inline void pack( Stream& s, const shared_ptr<const T>& v,
                                                            uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream, typename T> inline void unpack( Stream& s, shared_ptr<const T>& v,
                                                              uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename Stream> inline void pack( Stream& s, const bool& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename Stream> inline void unpack( Stream& s, bool& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );

    template<typename T> inline std::vector<char> pack( const T& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename T> inline T unpack( const std::vector<char>& s, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename T> inline T unpack( const char* d, uint32_t s, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
    template<typename T> inline void unpack( const char* d, uint32_t s, T& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH );
} }
