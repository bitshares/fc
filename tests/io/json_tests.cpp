#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/iostream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/sstream.hpp>

#include <fstream>

BOOST_AUTO_TEST_SUITE(json_tests)

static void replace_some( std::string& str )
{
   for( size_t i = 0; i < str.length(); i++ )
      if( str[i] == '\1' ) str[i] = '\0';
      else if( str[i] == '\'' ) str[i] = '"';
}

static void test_fail_string( const std::string& str )
{
   try {
      fc::json::from_string( str );
      BOOST_FAIL( "json::from_string('" + str + "') failed" );
   } catch( const fc::parse_error_exception& ) { // ignore, ok
   } catch( const fc::eof_exception& ) { // ignore, ok
   } FC_CAPTURE_LOG_AND_RETHROW( ("json::from_string failed")(str) )
}

static void test_fail_stream( const std::string& str )
{
   fc::temp_file file( fc::temp_directory_path(), true );
   {
      std::fstream init( file.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
      init.write( str.c_str(), str.length() );
      init.close();
   }
   try {
      fc::istream_ptr in( new fc::ifstream( file.path() ) );
      fc::buffered_istream bin( in );
      fc::json::from_stream( bin );
      BOOST_FAIL( "json::from_stream('" + str + "') failed using ifstream" );
   } catch( const fc::parse_error_exception& ) { // ignore, ok
   } catch( const fc::eof_exception& ) { // ignore, ok
   } FC_CAPTURE_LOG_AND_RETHROW( ("json::from_stream failed using ifstream")(str) )
   try {
      fc::istream_ptr in( new fc::stringstream( str ) );
      fc::buffered_istream bin( in );
      fc::json::from_stream( bin );
      BOOST_FAIL( "json::from_stream('" + str + "') failed using stringstream" );
   } catch( const fc::parse_error_exception& ) { // ignore, ok
   } catch( const fc::eof_exception& ) { // ignore, ok
   } FC_CAPTURE_LOG_AND_RETHROW( ("json::from_stream failed using stringstream")(str) )
}

static void test_fail_file( const std::string& str )
{
   fc::temp_file file( fc::temp_directory_path(), true );
   {
      std::fstream init( file.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
      init.write( str.c_str(), str.length() );
      init.close();
   }
   try {
      fc::json::from_file( file.path() );
      BOOST_FAIL( "json::from_file('" + str + "') failed using" );
   } catch( const fc::parse_error_exception& ) { // ignore, ok
   } catch( const fc::eof_exception& ) { // ignore, ok
   } FC_CAPTURE_LOG_AND_RETHROW( ("json::from_file failed")(str) )
}

BOOST_AUTO_TEST_CASE(imbalanced_test)
{
   std::string open40("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[");
   std::string close40("]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");
   std::string open80 = open40 + open40;
   std::string close80 = close40 + close40;
   std::vector<std::string> tests
   { // for easier handling and better readability, in the following test
     // strings ' is used instead of " and \1 instead of \0
      "",
      "{",
      "{'",
      "{'}",
      "{'a'",
      "{'a':",
      "{'a':5",
      "[",
      "['",
      "[']",
      "[ 13",
      "' end",
      "{ 13: }",
      "\1",
      "{\1",
      "{\1}",
      "{'\1",
      "{'\1}",
      "{'a'\1",
      "{'a'\1}",
      "{'a': \1",
      "{'a': \1}",
      "[\1",
      "[\1]",
      "['\1",
      "['\1]",
      "[ 13\1",
      "[ 13\1]",
      "' end\1",
      open80 + "'" + close80 + close80 + "'," + open80 + open80
             + close80 + close80 + close80
   };

   for( std::string test : tests )
   {
      replace_some( test );
      test_fail_string( test );
      test_fail_stream( test );
      test_fail_file( test );
   }
}

static bool equal( const fc::variant& a, const fc::variant& b )
{
   auto a_type = a.get_type();
   auto b_type = b.get_type();
   if( a_type == fc::variant::type_id::int64_type && a.as<int64_t>(1) > 0 )
       a_type = fc::variant::type_id::uint64_type;
   if( b_type == fc::variant::type_id::int64_type && b.as<int64_t>(1) > 0 )
       b_type = fc::variant::type_id::uint64_type;
   if( a_type != b_type )
   {
      if( ( a_type == fc::variant::type_id::double_type
            && b_type == fc::variant::type_id::string_type )
          || ( a_type == fc::variant::type_id::string_type
               && b_type == fc::variant::type_id::double_type ) )
         return a.as<double>(1) == b.as<double>(1);
      return false;
   }
   switch( a_type )
   {
      case fc::variant::type_id::null_type: return true;
      case fc::variant::type_id::int64_type: return a.as<int64_t>(1) == b.as<int64_t>(1);
      case fc::variant::type_id::uint64_type: return a.as<uint64_t>(1) == b.as<uint64_t>(1);
      case fc::variant::type_id::double_type: return a.as<double>(1) == b.as<double>(1);
      case fc::variant::type_id::bool_type: return a.as<bool>(1) == b.as<bool>(1);
      case fc::variant::type_id::string_type: return a.as<std::string>(1) == b.as<std::string>(1);
      case fc::variant::type_id::array_type:
         if( a.get_array().size() != b.get_array().size() ) return false;
         else
         {
            std::vector<fc::variant>::const_iterator b_it = b.get_array().begin();
            for( const auto& a_it : a.get_array() )
            {
               if( !equal( a_it, *b_it ) ) return false;
               b_it++;
            }
         }
         return true;
      case fc::variant::type_id::object_type:
         if( a.get_object().size() != b.get_object().size() ) return false;
         for( const auto& a_it : a.get_object() )
         {
            const auto& b_obj = b.get_object().find( a_it.key() );
            if( b_obj == b.get_object().end() || !equal( a_it.value(), b_obj->value() ) ) return false;
         }
         return true;
      case fc::variant::type_id::blob_type:
      default:
         FC_THROW_EXCEPTION( fc::invalid_arg_exception, "Unsupported variant type: {t}", ( "t", a.get_type() ) );
    }
}

static void test_recursive( const fc::variant& v )
{ try {
   const std::string json = fc::json::to_string( v );
   BOOST_CHECK( fc::json::is_valid( json ) );
   BOOST_CHECK( !fc::json::is_valid( json + " " ) );

   const std::string pretty = fc::json::to_pretty_string( v );
   BOOST_CHECK( fc::json::is_valid( pretty ) );
   BOOST_CHECK( !fc::json::is_valid( pretty + " " ) );

   fc::temp_file file( fc::temp_directory_path(), true );
   fc::json::save_to_file( v, file.path(), false );

   fc::variants list = fc::json::variants_from_string( json );
   BOOST_CHECK_EQUAL( 1u, list.size() );
   BOOST_CHECK( equal( v, list[0] ) );

   list = fc::json::variants_from_string( pretty );
   BOOST_CHECK_EQUAL( 1u, list.size() );
   BOOST_CHECK( equal( v, list[0] ) );

   BOOST_CHECK( equal( v, fc::json::from_string( json + " " ) ) );
   BOOST_CHECK( equal( v, fc::json::from_string( pretty + " " ) ) );
   BOOST_CHECK( equal( v, fc::json::from_file( file.path() ) ) );

   if( v.get_type() == fc::variant::type_id::array_type )
      for( const auto& item : v.get_array() )
          test_recursive( item );
   else if( v.get_type() == fc::variant::type_id::object_type )
      for( const auto& item : v.get_object() )
          test_recursive( item.value() );
} FC_CAPTURE_LOG_AND_RETHROW( (v) ) }

BOOST_AUTO_TEST_CASE(structured_test)
{
   fc::variant_object v_empty_obj;
   fc::variants v_empty_array;
   fc::variant v_null;
   fc::variant v_true( true );
   fc::variant v_false( false );
   fc::variant v_empty_str( "" );
   fc::variant v_str( "false" );
   fc::variant v_int8_1( (int8_t) 1 );
   fc::variant v_int8_2( (int8_t) -2 );
   fc::variant v_uint8_1( (int8_t) 1 );
   fc::variant v_int16_1( (int16_t) 1 );
   fc::variant v_int16_2( (int16_t) -2 );
   fc::variant v_uint16_1( (int16_t) 1 );
   fc::variant v_int32_1( (int32_t) 1 );
   fc::variant v_int32_2( (int32_t) -2 );
   fc::variant v_uint32_1( (int32_t) 1 );
   fc::variant v_int64_1( (int8_t) 1 );
   fc::variant v_int64_2( (int8_t) -2 );
   fc::variant v_uint64_1( (int8_t) 1 );
   fc::variant v_float_1( 0.0f );
   fc::variant v_float_2( -2.0f );
   fc::variant v_double_1( 0.0 );
   fc::variant v_double_2( -2.0 );
   fc::variants v_small_array
   {
       v_empty_obj,
       v_empty_array,
       v_null,
       v_true,
       v_false,
       v_empty_str
   };
   fc::mutable_variant_object v_small_obj;
   v_small_obj( "", v_empty_str )
              ( "1", v_empty_array )
              ( "2", v_null )
              ( "a", v_true )
              ( "b", v_false )
              ( "x", v_small_array )
              ( "y", v_empty_obj );
   fc::variants v_big_array
   {
       v_empty_obj,
       v_empty_array,
       v_null,
       v_true,
       v_false,
       v_empty_str,
       v_str,
       v_int8_1,
       v_int8_2,
       v_uint8_1,
       v_int16_1,
       v_int16_2,
       v_uint16_1,
       v_int32_1,
       v_int32_2,
       v_uint32_1,
       v_int64_1,
       v_int64_2,
       v_uint64_1,
       v_float_1,
       v_float_2,
       v_double_1,
       v_double_2,
       v_small_array,
       v_small_obj
   };
   fc::mutable_variant_object v_big_obj;
   v_big_obj( "v_empty_obj", v_empty_obj )
            ( "v_empty_array", v_empty_array )
            ( "v_null", v_null )
            ( "v_true", v_true )
            ( "v_false", v_false )
            ( "v_empty_str", v_empty_str )
            ( "v_str", v_str )
            ( "v_int8_1", v_int8_1 )
            ( "v_int8_2", v_int8_2 )
            ( "v_uint8_1", v_uint8_1 )
            ( "v_int16_1", v_int16_1 )
            ( "v_int16_2", v_int16_2 )
            ( "v_uint16_1", v_uint16_1 )
            ( "v_int32_1", v_int32_1 )
            ( "v_int32_2", v_int32_2 )
            ( "v_uint32_1", v_uint32_1 )
            ( "v_int64_1", v_int64_1 )
            ( "v_int64_2", v_int64_2 )
            ( "v_uint64_1", v_uint64_1 )
            ( "v_float_1", v_float_1 )
            ( "v_float_2", v_float_2 )
            ( "v_double_1", v_double_1 )
            ( "v_double_2", v_double_2 )
            ( "v_small_array", v_small_array )
            ( "v_small_obj", v_small_obj );
   v_big_array.push_back( v_big_obj );

   test_recursive( v_big_array );
}

BOOST_AUTO_TEST_CASE(precision_test)
{
   BOOST_CHECK_EQUAL( "\"4294967296\"", fc::json::to_string( fc::variant( int64_t(0x100000000LL) ) ) );
   BOOST_CHECK_EQUAL( "\"-4294967296\"", fc::json::to_string( fc::variant( int64_t(-0x100000000LL) ) ) );
   std::string half = fc::json::to_string( fc::variant( 0.5 ) );
   BOOST_CHECK_EQUAL( '"', half.front() );
   BOOST_CHECK_EQUAL( '"', half.back() );
   half = half.substr( 1, half.length() - 2 );
   while( '0' == half.back() ) half.erase( half.length() - 1, 1 );
   BOOST_CHECK_EQUAL( "0.5", half );
}

BOOST_AUTO_TEST_CASE(recursion_test)
{
   std::string ten_levels = "[[[[[[[[[[]]]]]]]]]]";
   fc::variant nested = fc::json::from_string( ten_levels );
   BOOST_CHECK_THROW( fc::json::from_string( ten_levels, fc::json::legacy_parser, 9 ), fc::parse_error_exception );

   std::string back = fc::json::to_string( nested );
   BOOST_CHECK_EQUAL( ten_levels, back );
   BOOST_CHECK_THROW( fc::json::to_string( nested, fc::json::stringify_large_ints_and_doubles, 9 ), fc::assert_exception );
}

BOOST_AUTO_TEST_CASE(rethrow_test)
{
   fc::variants biggie;
   for( int i = 0; i < 250; i++ )
   {
      fc::variant tmp( std::move(biggie) );
      biggie.reserve(1);
      biggie.push_back( std::move(tmp) );
   }

   auto test_r = [&biggie](){
      try {
         FC_THROW_EXCEPTION( fc::unknown_host_exception, "WTF?" );
      } FC_RETHROW_EXCEPTIONS( warn, "Argh! ${biggie}", ("biggie",biggie) ) };
   BOOST_CHECK_THROW( test_r(), fc::unknown_host_exception );

   auto test_lr = [](){
      try {
         FC_THROW_EXCEPTION( fc::unknown_host_exception, "WTF?" );
      } FC_LOG_AND_RETHROW() };
   BOOST_CHECK_THROW( test_lr(), fc::unknown_host_exception );

   auto test_clr = [&biggie](){
      try {
         FC_THROW_EXCEPTION( fc::unknown_host_exception, "WTF?" );
      } FC_CAPTURE_LOG_AND_RETHROW( (biggie) ) };
   BOOST_CHECK_THROW( test_clr(), fc::unknown_host_exception );

   auto test_cl = [&biggie](){
      try {
         FC_THROW_EXCEPTION( fc::unknown_host_exception, "WTF?" );
      } FC_CAPTURE_AND_LOG( (biggie) ) };
   test_cl();

   auto test_cr = [&biggie](){
      try {
         FC_THROW_EXCEPTION( fc::unknown_host_exception, "WTF?" );
      } FC_CAPTURE_AND_RETHROW( (biggie) ) };
   BOOST_CHECK_THROW( test_cr(), fc::unknown_host_exception );
}

BOOST_AUTO_TEST_SUITE_END()
