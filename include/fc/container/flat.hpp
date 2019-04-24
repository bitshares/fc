#pragma once
#include <fc/variant.hpp>
#include <fc/container/flat_fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {
   namespace raw {
       template<typename Stream, typename T, typename... A>
       inline void pack( Stream& s, const flat_set<T, A...>& value, uint32_t _max_depth ) {
         FC_ASSERT( _max_depth > 0 );
         --_max_depth;
         pack( s, unsigned_int(value.size()), _max_depth );
         auto itr = value.begin();
         auto end = value.end();
         while( itr != end ) {
           fc::raw::pack( s, *itr, _max_depth );
           ++itr;
         }
       }
       template<typename Stream, typename T, typename... A>
       inline void unpack( Stream& s, flat_set<T, A...>& value, uint32_t _max_depth ) {
         FC_ASSERT( _max_depth > 0 );
         --_max_depth;
         unsigned_int size; unpack( s, size, _max_depth );
         value.clear();
         value.reserve( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
         for( uint32_t i = 0; i < size.value; ++i )
         {
             T tmp;
             fc::raw::unpack( s, tmp, _max_depth );
             value.insert( std::move(tmp) );
         }
       }
       template<typename Stream, typename K, typename... V>
       inline void pack( Stream& s, const flat_map<K,V...>& value, uint32_t _max_depth ) {
         FC_ASSERT( _max_depth > 0 );
         --_max_depth;
         pack( s, unsigned_int(value.size()), _max_depth );
         auto itr = value.begin();
         auto end = value.end();
         while( itr != end ) {
           fc::raw::pack( s, *itr, _max_depth );
           ++itr;
         }
       }
       template<typename Stream, typename K, typename V, typename... A>
       inline void unpack( Stream& s, flat_map<K,V,A...>& value, uint32_t _max_depth )
       {
         FC_ASSERT( _max_depth > 0 );
         --_max_depth;
         unsigned_int size; unpack( s, size, _max_depth );
         value.clear();
         value.reserve( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
         for( uint32_t i = 0; i < size.value; ++i )
         {
             std::pair<K,V> tmp;
             fc::raw::unpack( s, tmp, _max_depth );
             value.insert( std::move(tmp) );
         }
       }

       template<typename Stream, typename T, typename A>
       void pack( Stream& s, const bip::vector<T,A>& value, uint32_t _max_depth ) {
         FC_ASSERT( _max_depth > 0 );
         --_max_depth;
         pack( s, unsigned_int(value.size()), _max_depth );
         if( !std::is_fundamental<T>::value ) {
            auto itr = value.begin();
            auto end = value.end();
            while( itr != end ) {
              fc::raw::pack( s, *itr, _max_depth );
              ++itr;
            }
         } else {
             s.write( (const char*)value.data(), value.size() );
         }
       }

       template<typename Stream, typename T, typename A>
       void unpack( Stream& s, bip::vector<T,A>& value, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          --_max_depth;
          unsigned_int size;
          unpack( s, size, _max_depth );
          if( !std::is_fundamental<T>::value ) {
             value.resize( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
             for( uint64_t i = 0; i < size; i++ )
             {
                if( i >= value.size() )
                   value.resize( std::min( static_cast<uint64_t>(2*value.size()), size.value ) );
                unpack( s, value[i], _max_depth );
             }
          } else {
             value.resize( size );
             s.read( (char*)value.data(), value.size() );
          }
       }

   } // namespace raw


   template<typename T, typename... A>
   void to_variant( const flat_set<T, A...>& var, variant& vo, uint32_t _max_depth )
   {
       FC_ASSERT( _max_depth > 0 );
      --_max_depth;
       std::vector<variant> vars(var.size());
       size_t i = 0;
       for( const auto& item : var )
          vars[i++] = variant( item, _max_depth );
       vo = vars;
   }
   template<typename T, typename... A>
   void from_variant( const variant& var, flat_set<T, A...>& vo, uint32_t _max_depth )
   {
      FC_ASSERT( _max_depth > 0 );
      --_max_depth;
      const variants& vars = var.get_array();
      vo.clear();
      vo.reserve( vars.size() );
      for( const auto& item : vars )
         vo.insert( item.as<T>(_max_depth) );
   }

   template<typename K, typename... T>
   void to_variant( const flat_map<K, T...>& var,  variant& vo, uint32_t _max_depth )
   {
       FC_ASSERT( _max_depth > 0 );
      --_max_depth;
       std::vector< variant > vars(var.size());
       size_t i = 0;
       for( const auto& item : var )
          vars[i++] = variant( item, _max_depth );
       vo = vars;
   }
   template<typename K, typename T, typename... A>
   void from_variant( const variant& var,  flat_map<K, T, A...>& vo, uint32_t _max_depth )
   {
      FC_ASSERT( _max_depth > 0 );
      --_max_depth;
      const variants& vars = var.get_array();
      vo.clear();
      for( const auto& item : vars )
         vo.insert( item.as<std::pair<K,T>>(_max_depth) );
   }

}
