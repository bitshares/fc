#include <boost/test/unit_test.hpp>

#include <fc/network/tcp_socket.hpp>
#include <fc/asio.hpp>

BOOST_AUTO_TEST_SUITE(tcp_tests)

/***
 * Running this test through valgrind will show
 * a memory leak due to lack of logic in destructor.
 * See https://github.com/bitshares/bitshares-fc/blob/51688042b0b9f99f03224f54fb937fe024fe5ced/src/asio.cpp#L152
 */
BOOST_AUTO_TEST_CASE(tcpconstructor_test)
{
   fc::tcp_socket socket;
}

class my_io_class : public fc::asio::default_io_service_scope
{
public:
   int16_t get_num_threads()
   {
      return get_default_num_threads();
   }
};

/***
 * Test the control of number of threads from outside
 */
BOOST_AUTO_TEST_CASE( number_threads_test )
{
   fc::asio::default_io_service_scope::set_default_num_threads(12);

   my_io_class my_class;

   BOOST_CHECK_EQUAL( 12, my_class.get_num_threads() );
}

/***
 * Test the control of number of threads from outside
 */
BOOST_AUTO_TEST_CASE( default_number_threads_test )
{
   // to erase leftovers from previous tests
   fc::asio::default_io_service_scope::set_default_num_threads(0);

   my_io_class my_class;

   fc::asio::default_io_service();

   BOOST_CHECK( my_class.get_num_threads() > 1 );
}

BOOST_AUTO_TEST_SUITE_END()
