#pragma once
#include <algorithm>
#include <fc/exception/exception.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/variant_object.hpp>
#include <fc/variant.hpp>

namespace fc { namespace raw {

    template<typename Stream>
    class variant_packer : public variant::visitor
    {
       public:
         variant_packer( Stream& _s, uint32_t _max_depth ):s(_s),max_depth(_max_depth - 1)
         {
            FC_ASSERT( _max_depth > 0 );
         }
         virtual void handle()const { }
         virtual void handle( const int64_t& v )const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const uint64_t& v )const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const double& v )const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const bool& v )const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const string& v )const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const variant_object& v)const
         {
            fc::raw::pack( s, v, max_depth );
         }
         virtual void handle( const variants& v)const
         {
            fc::raw::pack( s, v, max_depth );
         }

         Stream& s;
         const uint32_t max_depth;

    };


    template<typename Stream>
    inline void pack( Stream& s, const variant& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       pack( s, uint8_t(v.get_type()), _max_depth );
       v.visit( variant_packer<Stream>( s, _max_depth ) );
    }
    template<typename Stream>
    inline void unpack( Stream& s, variant& v, uint32_t _max_depth )
    {
      FC_ASSERT( _max_depth > 0 );
      --_max_depth;
      uint8_t t;
      unpack( s, t, _max_depth );
      switch( t )
      {
         case variant::null_type:
            return;
         case variant::int64_type:
         {
            int64_t val;
            raw::unpack( s, val, _max_depth );
            v = val;
            return;
         }
         case variant::uint64_type:
         {
            uint64_t val;
            raw::unpack( s, val, _max_depth );
            v = val;
            return;
         }
         case variant::double_type:
         {
            double val;
            raw::unpack( s, val, _max_depth );
            v = val;
            return;
         }
         case variant::bool_type:
         {
            bool val;
            raw::unpack( s, val, _max_depth );
            v = val;
            return;
         }
         case variant::string_type:
         {
            std::string val;
            raw::unpack( s, val, _max_depth );
            v = std::move(val);
            return;
         }
         case variant::array_type:
         {
            variants val;
            raw::unpack( s, val, _max_depth );
            v = std::move(val);
            return;
         }
         case variant::object_type:
         {
            variant_object val;
            raw::unpack( s, val, _max_depth );
            v = std::move(val);
            return;
         }
         default:
            FC_THROW_EXCEPTION( parse_error_exception, "Unknown Variant Type ${t}", ("t", t) );
      }
    }

    template<typename Stream>
    inline void pack( Stream& s, const variant_object& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int vs = v.size();
       pack( s, vs, _max_depth );
       for( auto itr = v.begin(); itr != v.end(); ++itr )
       {
          pack( s, itr->key(), _max_depth );
          pack( s, itr->value(), _max_depth );
       }
    }
    template<typename Stream>
    inline void unpack( Stream& s, variant_object& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int vs;
       unpack( s, vs, _max_depth );
       mutable_variant_object mvo;
       mvo.reserve( std::min( vs.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
       for( uint32_t i = 0; i < vs.value; ++i )
       {
          std::string key;
          fc::variant value;
          fc::raw::unpack( s, key, _max_depth );
          fc::raw::unpack( s, value, _max_depth );
          mvo.set( std::move(key), std::move(value) );
       }
       v = std::move(mvo);
    }

} } // fc::raw
