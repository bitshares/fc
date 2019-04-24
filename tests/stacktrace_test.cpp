#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string.hpp>

#include <signal.h>

#include <fc/stacktrace.hpp>
#include <fc/static_variant.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(fc_stacktrace)

BOOST_AUTO_TEST_CASE(stacktrace_test)
{
   // print the stack trace
   std::stringstream ss;
   fc::print_stacktrace(ss);
   std::string results = ss.str();
#if BOOST_VERSION / 100 >= 1065 && !defined(__APPLE__)
   BOOST_CHECK(!results.empty());
   BOOST_CHECK(results.find("fc::print_stacktrace") != std::string::npos);
#else
   BOOST_CHECK(results.empty());
#endif
}

BOOST_AUTO_TEST_CASE(threaded_stacktrace_test)
{
   fc::thread test_thread("a_thread");
   std::string results = test_thread.async(
         [] ()->std::string {
               // cause a pause
               for(int i = 0; i < 10000; i++);
               std::stringstream ss;
               fc::print_stacktrace(ss);
               return ss.str();
            }
         ).wait();
#if BOOST_VERSION / 100 >= 1065 && !defined(__APPLE__)
   BOOST_CHECK(!results.empty());
   BOOST_CHECK(results.find("fc::print_stacktrace") != std::string::npos);
#else
   BOOST_CHECK(results.empty());
#endif
}

#if BOOST_VERSION / 100 >= 1065 && !defined(__APPLE__)
class _svdt_visitor
{
public:
   typedef std::string result_type;
   std::string operator()( int64_t i )const
   {
      std::stringstream ss;
      fc::print_stacktrace(ss);
      return ss.str();
   }
   template<typename T>
   std::string operator()( T i )const { return "Unexpected!"; }
};

BOOST_AUTO_TEST_CASE(static_variant_depth_test)
{
   int64_t i = 1;
   fc::static_variant<uint8_t,uint16_t,uint32_t,uint64_t,int8_t,int16_t,int32_t,int64_t> test(i);

   std::string stacktrace = test.visit( _svdt_visitor() );
   //std::cerr << stacktrace << "\n";
   std::vector<std::string> lines;
   boost::split( lines, stacktrace, boost::is_any_of("\n") );
   int count = 0;
   for( const auto& line : lines )
      if( line.find("_svdt_visitor") != std::string::npos ) count++;
   BOOST_CHECK_LT( 2, count ); // test.visit(), static_variant::visit, function object, visitor
   BOOST_CHECK_GT( 8, count ); // some is implementation-dependent
}
#endif

/* this test causes a segfault on purpose to test the event handler
BOOST_AUTO_TEST_CASE(cause_segfault)
{
   fc::print_stacktrace_on_segfault();
   ::raise(SIGSEGV);
}
*/
BOOST_AUTO_TEST_SUITE_END()
