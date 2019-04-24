#include <boost/test/unit_test.hpp>
#include <fc/log/logger.hpp>

#include <fc/container/flat.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/static_variant.hpp>
#include <fc/log/logger_config.hpp>

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

BOOST_AUTO_TEST_SUITE(fc_variant_and_log)

BOOST_AUTO_TEST_CASE( types_edge_cases_test )
{
   using namespace fc::test;

   class sv : public fc::static_variant<>
   {
   public:
       BOOST_ATTRIBUTE_UNUSED typedef fc::static_variant<>::tag_type tag_type;
   };

   BOOST_TEST_MESSAGE( "========== Test empty static_variant ==========" );

   BOOST_CHECK_THROW( fc::static_variant<>(), fc::assert_exception );

   BOOST_TEST_MESSAGE( "========== Test static_variant with tag_type ==========" );

   sv::tag_type init_value = 2;
   fc::static_variant< sv::tag_type, std::string, fc::variant > variant_with_tagtype(init_value);

   BOOST_CHECK_EQUAL( variant_with_tagtype.count(), 3 );
   BOOST_CHECK_EQUAL( variant_with_tagtype.which(), 0 );

   sv::tag_type current_value = variant_with_tagtype.get<sv::tag_type>();
   BOOST_CHECK_EQUAL( current_value, init_value );
   BOOST_CHECK( variant_with_tagtype.get<sv::tag_type>() == init_value );

   for (sv::tag_type i = variant_with_tagtype.count(); i-->0;)
   {
      variant_with_tagtype.set_which(i);
      BOOST_CHECK_EQUAL(variant_with_tagtype.which(), i);
   }

   BOOST_TEST_MESSAGE( "========== Test static_variant with static_variant ==========" );

   using sv_double = fc::static_variant<double>;
   using sv_float = fc::static_variant<float>;
   fc::static_variant< sv_float, std::string, sv_double > variant;
   sv_float variant_float = 1.5f;
   variant = variant_float;
   BOOST_CHECK_EQUAL( variant.which(), 0 );
   BOOST_CHECK_EQUAL( variant.get<sv_float>().get<float>(), 1.5f );

   sv_double variant_double = 1.0;
   variant = variant_double;
   BOOST_CHECK_EQUAL( variant.which() , 2);
   BOOST_CHECK_EQUAL( variant.get<sv_double>().get<double>(), 1.0 );
}


BOOST_AUTO_TEST_CASE( nested_objects_test )
{ try {

   ilog( "Suppressing logging (but not disabled)" );
   fc::logging_config cfg;
   fc::logger_config dlc;
   dlc.name = "default";
   dlc.level = fc::log_level::debug;
   dlc.appenders.push_back("stderr");
   cfg.loggers.push_back( dlc );
   fc::configure_logging( cfg );

   auto create_nested_object = []( uint32_t level )
   {
      ilog( "Creating nested object with ${lv} level(s)", ("lv",level) );
      fc::test::item nested;
      for( uint32_t i = 1; i <= level; i++ )
      {
         if( i % 10 == 0 )
            ilog( "Creating level ${lv}", ("lv",i) );
         fc::test::item_wrapper wp( std::move(nested) );
         nested = fc::test::item( std::move(wp), i );
      }
      return nested;
   };

   for (int nested_loops = 0; nested_loops <= 100; ++nested_loops )
   {
      int nested_levels = nested_loops * 3 + 2;
      auto nested = create_nested_object( nested_loops );

      fc::variant v;

      BOOST_TEST_MESSAGE( "========== About to nested object convert to variant. ==========" );
      BOOST_CHECK_THROW( to_variant( nested, v, nested_levels ), fc::assert_exception );
      try{ try { try { try {
         to_variant( nested, v, nested_levels ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (nested) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (nested) ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (nested) ); // CAPTURE_AND_LOG should never throw again

      to_variant( nested, v, nested_levels + 1 );

      BOOST_TEST_MESSAGE( "========== About to convert nested object from variant. ==========" );
      fc::test::item unpacked;
      BOOST_CHECK_THROW( from_variant( v, unpacked, nested_levels ), fc::assert_exception );
      try{ try { try { try {
         from_variant( v, unpacked, nested_levels ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (v) ); // CAPTURE_AND_LOG should never throw again

      from_variant( v, unpacked, nested_levels + 1 );

      BOOST_CHECK( unpacked == nested );

      // both log and dump should never throw
      BOOST_TEST_MESSAGE( "========== About to log obj. ==========" );
      ilog( "The obj is ${a}, variant is ${v}", ("a",nested)("v",v) );
      BOOST_TEST_MESSAGE( "========== About to dump obj. ==========" );
      idump( (nested)(v) );

      fc::static_variant<fc::test::item> sv( nested ), sv1;
      BOOST_TEST_MESSAGE( "========== About to convert static_variant to variant. ==========" );
      BOOST_CHECK_THROW( to_variant( sv, v, nested_levels + 1 ), fc::assert_exception );
      try{ try { try { try {
         to_variant( sv, v, nested_levels + 1 ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (sv) ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (sv) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (sv) ); // CAPTURE_AND_LOG should never throw again

      to_variant( sv, v, nested_levels + 2 );

      BOOST_TEST_MESSAGE( "========== About to convert static_variant from variant. ==========" );
      BOOST_CHECK_THROW( from_variant( v, sv1, nested_levels + 1 ), fc::assert_exception );
      try{ try { try { try {
         from_variant( v, sv1, nested_levels + 1 ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (v) ); // CAPTURE_AND_LOG should never throw again

      from_variant( v, sv1, nested_levels + 2 );

      BOOST_CHECK( sv == sv1 );

      // both log and dump should never throw
      BOOST_TEST_MESSAGE( "========== About to log static_variant. ==========" );
      wlog( "The static_variant is ${a}, variant is $(v)", ("a",sv)("v",v) );
      BOOST_TEST_MESSAGE( "========== About to dump static_variant. ==========" );
      wdump( (sv)(v) );

      std::vector<fc::static_variant<fc::test::item>> vec(300), vec1;
      for( int i = 0; i < 300; i++ )
         vec.push_back(sv);
      BOOST_TEST_MESSAGE( "========== About to convert vector to variant. ==========" );
      BOOST_CHECK_THROW( to_variant( vec, v, nested_levels + 2 ), fc::assert_exception );
      try{ try { try { try {
         to_variant( vec, v, nested_levels + 2 ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (vec) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (vec) ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (vec) ); // CAPTURE_AND_LOG should never throw again

      to_variant( vec, v, nested_levels + 3 );

      BOOST_TEST_MESSAGE( "========== About to convert vector from variant. ==========" );
      BOOST_CHECK_THROW( from_variant( v, vec1, nested_levels + 2 ), fc::assert_exception );
      try{ try { try { try {
         from_variant( v, vec1, nested_levels + 2 ); BOOST_FAIL( "Expected exception." );
      } FC_LOG_AND_RETHROW( ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_LOG_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_RETHROW( (v) ); BOOST_FAIL( "Expected exception." );
      } FC_CAPTURE_AND_LOG ( (v) ); // CAPTURE_AND_LOG should never throw again

      from_variant( v, vec1, nested_levels + 3 );

      BOOST_CHECK( vec == vec1 );

      // both log and dump should never throw
      BOOST_TEST_MESSAGE( "========== About to log vector. ==========" );
      elog( "The vector is ${a}, variant is ${v}", ("a",vec)("v",v) );
      BOOST_TEST_MESSAGE( "========== About to dump vector. ==========" );
      edump( (vec)(v) );

   }

   fc::configure_logging( fc::logging_config::default_config() );
   ilog( "End suppressing logging" );

} FC_CAPTURE_LOG_AND_RETHROW ( (0) ) }

BOOST_AUTO_TEST_SUITE_END()
