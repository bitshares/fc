#include <boost/test/unit_test.hpp>
#include <fc/log/logger.hpp>

#include <fc/container/flat.hpp>
#include <fc/io/raw.hpp>

namespace fc { namespace test {

   struct item;
   inline bool operator == ( const item& a, const item& b );
   inline bool operator < ( const item& a, const item& b );

   struct item_wrapper
   {
      item_wrapper() {}
      item_wrapper(item&& it) { v.reserve(1); v.insert( it ); }
      boost::container::flat_set<struct item> v;
   };

   inline bool operator == ( const item_wrapper& a, const item_wrapper& b )
   { return ( std::tie( a.v ) == std::tie( b.v ) ); }
   inline bool operator < ( const item_wrapper& a, const item_wrapper& b )
   { return ( std::tie( a.v ) < std::tie( b.v ) ); }

   struct item
   {
      item(int32_t lvl = 0) : level(lvl) {}
      item(item_wrapper&& wp, int32_t lvl = 0) : level(lvl), w(wp) {}
      int32_t      level;
      item_wrapper w;
   };

   inline bool operator == ( const item& a, const item& b )
   { return ( std::tie( a.level, a.w ) == std::tie( b.level, b.w ) ); }
   inline bool operator < ( const item& a, const item& b )
   { return ( std::tie( a.level, a.w ) < std::tie( b.level, b.w ) ); }


} } // namespace fc::test

FC_REFLECT( fc::test::item_wrapper, (v) );
FC_REFLECT( fc::test::item, (level)(w) );

BOOST_AUTO_TEST_SUITE(fc_serialization)

BOOST_AUTO_TEST_CASE( nested_objects_test )
{ try {

   auto create_nested_object = []( uint32_t level )
   {
      ilog( "Creating nested object with ${lv} level(s)", ("lv",level) );
      fc::test::item nested;
      for( uint32_t i = 1; i <= level; i++ )
      {
         if( i % 100 == 0 )
            ilog( "Creating level ${lv}", ("lv",i) );
         fc::test::item_wrapper wp( std::move(nested) );
         nested = fc::test::item( std::move(wp), i );
      }
      return nested;
   };

   // 100 levels, should be allowed
   {
      auto nested = create_nested_object( 100 );

      std::stringstream ss;

      BOOST_TEST_MESSAGE( "About to pack." );
      fc::raw::pack( ss, nested );

      BOOST_TEST_MESSAGE( "About to unpack." );
      fc::test::item unpacked;
      fc::raw::unpack( ss, unpacked );

      BOOST_CHECK( unpacked == nested );
   }

   // 150 levels, by default packing will fail
   {
      auto nested = create_nested_object( 150 );

      std::stringstream ss;

      BOOST_TEST_MESSAGE( "About to pack." );
      BOOST_CHECK_THROW( fc::raw::pack( ss, nested ), fc::assert_exception );
   }

   // 150 levels and allow packing, unpacking will fail
   {
      auto nested = create_nested_object( 150 );

      std::stringstream ss;

      BOOST_TEST_MESSAGE( "About to pack." );
      fc::raw::pack( ss, nested, 1500 );

      BOOST_TEST_MESSAGE( "About to unpack." );
      fc::test::item unpacked;
      BOOST_CHECK_THROW( fc::raw::unpack( ss, unpacked ), fc::assert_exception );
   }

} FC_CAPTURE_LOG_AND_RETHROW ( (0) ) }

BOOST_AUTO_TEST_CASE( unpack_recursion_test )
{
   try
   {
      std::stringstream ss;
      int recursion_level = 100000;
      uint64_t allocation_per_level = 500000;

      for ( int i = 0; i < recursion_level; i++ )
      {
         fc::raw::pack( ss, fc::unsigned_int( allocation_per_level ) );
         fc::raw::pack( ss, static_cast< uint8_t >( fc::variant::array_type ) );
      }

      std::vector< fc::variant > v;
      BOOST_REQUIRE_THROW( fc::raw::unpack( ss, v ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
