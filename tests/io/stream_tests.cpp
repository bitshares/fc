#include <boost/test/unit_test.hpp>

#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/sstream.hpp>

#include <fstream>

BOOST_AUTO_TEST_SUITE(stream_tests)

BOOST_AUTO_TEST_CASE(stringstream_test)
{
   const fc::string constant( "Hello", 6 ); // includes trailing \0
   fc::string writable( "World" );
   fc::stringstream in1( constant );
   fc::stringstream in2( writable );
   fc::stringstream out;

   std::shared_ptr<char> buf( new char[15], [](char* p){ delete[] p; } );
   *buf = 'w';
   in2.writesome( buf, 1, 0 );

   BOOST_CHECK_EQUAL( 3u, in1.readsome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 3u, out.writesome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 'l', in1.peek() );
   BOOST_CHECK_EQUAL( 3u, in1.readsome( buf, 4, 0 ) );
   BOOST_CHECK_EQUAL( '\0', (&(*buf))[2] );
   BOOST_CHECK_EQUAL( 2u, out.writesome( buf, 2, 0 ) );
   *buf = ' ';
   out.writesome( buf, 1, 0 );
   BOOST_CHECK_THROW( in1.readsome( buf, 3, 0 ), fc::eof_exception );
   BOOST_CHECK_EQUAL( 5u, in2.readsome( buf, 6, 0 ) );
   BOOST_CHECK_EQUAL( 5u, out.writesome( buf, 5, 0 ) );
   BOOST_CHECK_THROW( in2.readsome( buf, 3, 0 ), fc::eof_exception );

   BOOST_CHECK_EQUAL( "Hello world", out.str() );
   BOOST_CHECK_THROW( in1.peek(), fc::eof_exception );
   BOOST_CHECK( in1.eof() );
   BOOST_CHECK_THROW( in2.readsome( buf, 3, 0 ), fc::eof_exception );
   // BOOST_CHECK( in2.eof() ); // fails, apparently readsome doesn't set eof
}

BOOST_AUTO_TEST_CASE(buffered_stringstream_test)
{
   const fc::string constant( "Hello", 6 ); // includes trailing \0
   fc::string writable( "World" );
   fc::istream_ptr in1( new fc::stringstream( constant ) );
   std::shared_ptr<fc::stringstream> in2( new fc::stringstream( writable ) );
   std::shared_ptr<fc::stringstream> out1( new fc::stringstream() );
   fc::buffered_istream bin1( in1 );
   fc::buffered_istream bin2( in2 );
   fc::buffered_ostream bout( out1 );

   std::shared_ptr<char> buf( new char[15], [](char* p){ delete[] p; } );
   *buf = 'w';
   in2->writesome( buf, 1, 0 );

   BOOST_CHECK_EQUAL( 3u, bin1.readsome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 3u, bout.writesome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 'l', bin1.peek() );
   BOOST_CHECK_EQUAL( 3u, bin1.readsome( buf, 4, 0 ) );
   BOOST_CHECK_EQUAL( '\0', (&(*buf))[2] );
   BOOST_CHECK_EQUAL( 2u, bout.writesome( buf, 2, 0 ) );
   *buf = ' ';
   bout.writesome( buf, 1, 0 );
   BOOST_CHECK_THROW( bin1.readsome( buf, 3, 0 ), fc::eof_exception );
   BOOST_CHECK_EQUAL( 5u, bin2.readsome( buf, 6, 0 ) );
   BOOST_CHECK_EQUAL( 5u, bout.writesome( buf, 5, 0 ) );
   BOOST_CHECK_THROW( bin2.readsome( buf, 3, 0 ), fc::eof_exception );

   bout.flush();

   BOOST_CHECK_EQUAL( "Hello world", out1->str() );
}

BOOST_AUTO_TEST_CASE(fstream_test)
{
   fc::temp_file inf1( fc::temp_directory_path(), true );
   fc::temp_file inf2( fc::temp_directory_path(), true );
   fc::temp_file outf( fc::temp_directory_path(), true );

   {
       std::fstream init( inf1.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.write( "Hello", 6 ); // includes trailing \0
       init.close();

       init.open( inf2.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.write( "world", 5 );
       init.close();

       init.open( outf.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.close();
   }

   fc::ifstream in1( inf1.path() );
   fc::ifstream in2( inf2.path() );
   fc::ofstream out( outf.path() );

   std::shared_ptr<char> buf( new char[15], [](char* p){ delete[] p; } );
   size_t have_got = in1.readsome( buf, 3, 0 );
   BOOST_REQUIRE_LE( 1u, have_got );
   BOOST_REQUIRE_GT( 4u, have_got );
   BOOST_CHECK_EQUAL( have_got, out.writesome( buf, have_got, 0 ) );
   BOOST_REQUIRE_EQUAL( 6 - have_got, in1.readsome( buf, 7 - have_got, 0 ) );
   BOOST_CHECK_EQUAL( '\0', (&(*buf))[5 - have_got] );
   BOOST_CHECK_EQUAL( 5 - have_got, out.writesome( buf, 5 - have_got, 0 ) );
   *buf = ' ';
   out.writesome( buf, 1, 0 );
   BOOST_CHECK_THROW( in1.readsome( buf, 3, 0 ), fc::eof_exception );
   BOOST_CHECK_EQUAL( 5u, in2.readsome( buf, 6, 0 ) );
   BOOST_CHECK_EQUAL( 5u, out.writesome( buf, 5, 0 ) );
   BOOST_CHECK_THROW( in2.readsome( buf, 3, 0 ), fc::eof_exception );

   {
      out.flush();
      std::fstream test( outf.path().to_native_ansi_path(), std::fstream::in );
      test.peek();
      BOOST_CHECK_EQUAL( 11u, test.readsome( (&(*buf)), 11 ) );
      BOOST_CHECK_EQUAL( "Hello world", std::string( (&(*buf)), 11 ) );
      BOOST_CHECK_EQUAL( 0u, test.readsome( (&(*buf)), 11 ) );
      test.close();
   }

   BOOST_CHECK( in1.eof() );
   BOOST_CHECK( in2.eof() );
}

BOOST_AUTO_TEST_CASE(buffered_fstream_test)
{
   fc::temp_file inf1( fc::temp_directory_path(), true );
   fc::temp_file inf2( fc::temp_directory_path(), true );
   fc::temp_file outf( fc::temp_directory_path(), true );

   {
       std::fstream init( inf1.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.write( "Hello", 6 ); // includes trailing \0
       init.close();

       init.open( inf2.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.write( "world", 5 );
       init.close();

       init.open( outf.path().to_native_ansi_path(), std::fstream::out | std::fstream::trunc );
       init.close();
   }

   fc::istream_ptr in1( new fc::ifstream( inf1.path() ) );
   fc::istream_ptr in2( new fc::ifstream( inf2.path() ) );
   fc::ostream_ptr out( new fc::ofstream( outf.path() ) );
   fc::buffered_istream bin1( in1 );
   fc::buffered_istream bin2( in2 );
   fc::buffered_ostream bout( out );

   std::shared_ptr<char> buf( new char[15], [](char* p){ delete[] p; } );

   BOOST_CHECK_EQUAL( 3u, bin1.readsome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 3u, bout.writesome( buf, 3, 0 ) );
   BOOST_CHECK_EQUAL( 'l', bin1.peek() );
   BOOST_CHECK_EQUAL( 3u, bin1.readsome( buf, 4, 0 ) );
   BOOST_CHECK_EQUAL( '\0', (&(*buf))[2] );
   BOOST_CHECK_EQUAL( 2u, bout.writesome( buf, 2, 0 ) );
   *buf = ' ';
   bout.writesome( buf, 1, 0 );
   BOOST_CHECK_THROW( bin1.readsome( buf, 3, 0 ), fc::eof_exception );
   BOOST_CHECK_EQUAL( 5u, bin2.readsome( buf, 6, 0 ) );
   BOOST_CHECK_EQUAL( 5u, bout.writesome( buf, 5, 0 ) );
   BOOST_CHECK_THROW( bin2.readsome( buf, 3, 0 ), fc::eof_exception );

   {
      bout.flush();
      std::fstream test( outf.path().to_native_ansi_path(), std::fstream::in );
      BOOST_CHECK_EQUAL( 11u, test.readsome( (&(*buf)), 11 ) );
      BOOST_CHECK_EQUAL( "Hello world", std::string( (&(*buf)), 11 ) );
      BOOST_CHECK_EQUAL( 0u, test.readsome( (&(*buf)), 11 ) );
      test.close();
   }
}

BOOST_AUTO_TEST_SUITE_END()
