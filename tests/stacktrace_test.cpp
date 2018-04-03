#include <boost/test/unit_test.hpp>

#include <fc/stacktrace.hpp>
#include <fc/thread/thread.hpp>

BOOST_AUTO_TEST_SUITE(fc_stacktrace)

BOOST_AUTO_TEST_CASE(stacktrace_test)
{
   // print the stack trace
   //print_stacktrace(std::ostream& out, unsigned int max_frames = 63, void* caller_overwrite_hack = nullptr)
   std::stringstream ss;
   fc::print_stacktrace(ss);
   std::string results = ss.str();
   BOOST_CHECK(!results.empty());
   BOOST_CHECK(results.find("fc::print_stacktrace") != std::string::npos);
   //std::cout << results << '\n';
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
   BOOST_CHECK(!results.empty());
   BOOST_CHECK(results.find("fc::print_stacktrace") != std::string::npos);
   //std::cout << results << '\n';
}

BOOST_AUTO_TEST_SUITE_END()
