/*
 * Copyright (c) 2018 The BitShares Blockchain, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/thread/parallel.hpp>
#include <fc/time.hpp>

#include <iostream>

struct thread_config {
  thread_config() {
     for( int i = 0; i < boost::unit_test::framework::master_test_suite().argc - 1; ++i )
        if( !strcmp( boost::unit_test::framework::master_test_suite().argv[i], "--pool-threads" ) )
        {
           uint16_t threads = atoi(boost::unit_test::framework::master_test_suite().argv[++i]);
           std::cout << "Using " << threads << " pool threads\n";
           fc::asio::default_io_service_scope::set_num_threads(threads);
        }
  }
};

BOOST_GLOBAL_FIXTURE( thread_config );


BOOST_AUTO_TEST_SUITE(parallel_tests)

BOOST_AUTO_TEST_CASE( do_nothing_parallel )
{
   std::vector<fc::future<void>> results;
   results.reserve( 20 );
   for( size_t i = 0; i < results.capacity(); i++ )
      results.push_back( fc::do_parallel( [i] () { std::cout << i << ","; } ) );
   for( auto& result : results )
      result.wait();
   std::cout << "\n";
}

BOOST_AUTO_TEST_CASE( do_something_parallel )
{
   struct result {
      boost::thread::id thread_id;
      int               call_count;
   };

   std::vector<fc::future<result>> results;
   results.reserve( 20 );
   boost::thread_specific_ptr<int> tls;
   for( size_t i = 0; i < results.capacity(); i++ )
      results.push_back( fc::do_parallel( [&tls] () {
         if( !tls.get() ) { tls.reset( new int(0) ); }
         result res = { boost::this_thread::get_id(), (*tls.get())++ };
         return res;
      } ) );

   std::map<boost::thread::id,std::vector<int>> results_by_thread;
   for( auto& res : results )
   {
      result r = res.wait();
      results_by_thread[r.thread_id].push_back( r.call_count );
   }

   BOOST_CHECK( results_by_thread.size() > 1 ); // require execution by more than 1 thread
   for( auto& pair : results_by_thread )
   {  // check that thread_local_storage counter works
      std::sort( pair.second.begin(), pair.second.end() );
      for( size_t i = 0; i < pair.second.size(); i++ )
         BOOST_CHECK_EQUAL( (int64_t)i, pair.second[i] );
   }
}

const std::string TEXT = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"$%&/()=?,.-#+´{[]}`*'_:;<>|";

template<typename Hash>
class hash_test {
   public:
      std::string _hashname = fc::get_typename<Hash>::name();

      void run_single_threaded() {
         const std::string first = Hash::hash(TEXT).str();
         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < 1000; i++ )
            BOOST_CHECK_EQUAL( first, Hash::hash(TEXT).str() );
         fc::time_point end = fc::time_point::now();
         ilog( "${c} single-threaded ${h}'s in ${t}µs", ("c",1000)("h",_hashname)("t",end-start) );
      }

      void run_multi_threaded() {
         const std::string first = Hash::hash(TEXT).str();
         std::vector<fc::future<std::string>> results;
         results.reserve( 10000 );
         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < 10000; i++ )
            results.push_back( fc::do_parallel( [] () { return Hash::hash(TEXT).str(); } ) );
         for( auto& result: results )
            BOOST_CHECK_EQUAL( first, result.wait() );
         fc::time_point end = fc::time_point::now();
         ilog( "${c} multi-threaded ${h}'s in ${t}µs", ("c",10000)("h",_hashname)("t",end-start) );
      }

      void run() {
         run_single_threaded();
         run_multi_threaded();
      }
};

BOOST_AUTO_TEST_CASE( hash_parallel )
{
   hash_test<fc::ripemd160>().run();
   hash_test<fc::sha1>().run();
   hash_test<fc::sha224>().run();
   hash_test<fc::sha256>().run();
   hash_test<fc::sha512>().run();
}

BOOST_AUTO_TEST_CASE( sign_verify_parallel )
{
   const fc::sha256 HASH = fc::sha256::hash(TEXT);

   std::vector<fc::ecc::private_key> keys;
   keys.reserve(1000);
   for( int i = 0; i < 1000; i++ )
      keys.push_back( fc::ecc::private_key::regenerate( fc::sha256::hash( TEXT + fc::to_string(i) ) ) );

   std::vector<fc::ecc::compact_signature> sigs;
   sigs.reserve( 10 * keys.size() );
   {
      fc::time_point start = fc::time_point::now();
      for( int i = 0; i < 10; i++ )
         for( const auto& key: keys )
            sigs.push_back( key.sign_compact( HASH ) );
      fc::time_point end = fc::time_point::now();
      ilog( "${c} single-threaded signatures in ${t}µs", ("c",sigs.size())("t",end-start) );
   }

   {
      fc::time_point start = fc::time_point::now();
      for( size_t i = 0; i < sigs.size(); i++ )
         BOOST_CHECK( keys[i % keys.size()].get_public_key() == fc::ecc::public_key( sigs[i], HASH ) );
      fc::time_point end = fc::time_point::now();
      ilog( "${c} single-threaded verifies in ${t}µs", ("c",sigs.size())("t",end-start) );
   }

   {
      std::vector<fc::future<fc::ecc::compact_signature>> results;
      results.reserve( 10 * keys.size() );
      fc::time_point start = fc::time_point::now();
      for( int i = 0; i < 10; i++ )
         for( const auto& key: keys )
            results.push_back( fc::do_parallel( [&key,&HASH] () { return key.sign_compact( HASH ); } ) );
      for( auto& res : results )
         res.wait();
      fc::time_point end = fc::time_point::now();
      ilog( "${c} multi-threaded signatures in ${t}µs", ("c",sigs.size())("t",end-start) );
   }

   {
      std::vector<fc::future<fc::ecc::public_key>> results;
      results.reserve( sigs.size() );
      fc::time_point start = fc::time_point::now();
      for( const auto& sig: sigs )
         results.push_back( fc::do_parallel( [&sig,&HASH] () { return fc::ecc::public_key( sig, HASH ); } ) );
      for( size_t i = 0; i < results.size(); i++ )
         BOOST_CHECK( keys[i % keys.size()].get_public_key() == results[i].wait() );
      fc::time_point end = fc::time_point::now();
      ilog( "${c} multi-threaded verifies in ${t}µs", ("c",sigs.size())("t",end-start) );
   }
}

BOOST_AUTO_TEST_CASE( serial_valve )
{
   boost::atomic<uint32_t> counter(0);
   fc::serial_valve valve;

   { // Simple test, f2 finishes before f1
      fc::promise<void>* syncer = new fc::promise<void>();
      fc::promise<void>* waiter = new fc::promise<void>();
      auto p1 = fc::async([&counter,&valve,syncer,waiter] () {
         valve.do_serial( [syncer,waiter](){ syncer->set_value();
                                             fc::future<void>( fc::shared_ptr<fc::promise<void>>( waiter, true ) ).wait(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 0u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      // at this point, p1.f1 has started executing and is waiting on waiter

      syncer = new fc::promise<void>();
      auto p2 = fc::async([&counter,&valve,syncer] () {
         valve.do_serial( [syncer](){ syncer->set_value(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 1u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      fc::usleep( fc::milliseconds(10) );

      // at this point, p2.f1 has started executing and p2.f2 is waiting for its turn

      BOOST_CHECK( !p1.ready() );
      BOOST_CHECK( !p2.ready() );

      waiter->set_value(); // signal p1.f1 to continue

      p2.wait(); // and wait for p2.f2 to complete

      BOOST_CHECK( p1.ready() );
      BOOST_CHECK( p2.ready() );
      BOOST_CHECK_EQUAL( 2u, counter.load() );
   }

   { // Triple test, f3 finishes first, then f1, finally f2
      fc::promise<void>* syncer = new fc::promise<void>();
      fc::promise<void>* waiter = new fc::promise<void>();
      counter.store(0);
      auto p1 = fc::async([&counter,&valve,syncer,waiter] () {
         valve.do_serial( [&syncer,waiter](){ syncer->set_value();
                                              fc::future<void>( fc::shared_ptr<fc::promise<void>>( waiter, true ) ).wait(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 0u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      // at this point, p1.f1 has started executing and is waiting on waiter

      syncer = new fc::promise<void>();
      auto p2 = fc::async([&counter,&valve,syncer] () {
         valve.do_serial( [&syncer](){ syncer->set_value();
                                       fc::usleep( fc::milliseconds(100) ); },
                          [&counter](){ BOOST_CHECK_EQUAL( 1u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      // at this point, p2.f1 has started executing and is sleeping

      syncer = new fc::promise<void>();
      auto p3 = fc::async([&counter,&valve,syncer] () {
         valve.do_serial( [syncer](){ syncer->set_value(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 2u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      fc::usleep( fc::milliseconds(10) );

      // at this point, p3.f1 has started executing and p3.f2 is waiting for its turn

      BOOST_CHECK( !p1.ready() );
      BOOST_CHECK( !p2.ready() );
      BOOST_CHECK( !p3.ready() );

      waiter->set_value(); // signal p1.f1 to continue

      p3.wait(); // and wait for p3.f2 to complete

      BOOST_CHECK( p1.ready() );
      BOOST_CHECK( p2.ready() );
      BOOST_CHECK( p3.ready() );
      BOOST_CHECK_EQUAL( 3u, counter.load() );
   }

   { // Triple test again but with invocations from different threads
      fc::promise<void>* syncer = new fc::promise<void>();
      fc::promise<void>* waiter = new fc::promise<void>();
      counter.store(0);
      auto p1 = fc::do_parallel([&counter,&valve,syncer,waiter] () {
         valve.do_serial( [&syncer,waiter](){ syncer->set_value();
                                              fc::future<void>( fc::shared_ptr<fc::promise<void>>( waiter, true ) ).wait(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 0u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      // at this point, p1.f1 has started executing and is waiting on waiter

      syncer = new fc::promise<void>();
      auto p2 = fc::do_parallel([&counter,&valve,syncer] () {
         valve.do_serial( [&syncer](){ syncer->set_value();
                                       fc::usleep( fc::milliseconds(100) ); },
                          [&counter](){ BOOST_CHECK_EQUAL( 1u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      // at this point, p2.f1 has started executing and is sleeping

      syncer = new fc::promise<void>();
      auto p3 = fc::do_parallel([&counter,&valve,syncer] () {
         valve.do_serial( [syncer](){ syncer->set_value(); },
                          [&counter](){ BOOST_CHECK_EQUAL( 2u, counter.load() );
                                        counter.fetch_add(1); } );
      });
      fc::future<void>( fc::shared_ptr<fc::promise<void>>( syncer, true ) ).wait();

      fc::usleep( fc::milliseconds(10) );

      // at this point, p3.f1 has started executing and p3.f2 is waiting for its turn

      BOOST_CHECK( !p1.ready() );
      BOOST_CHECK( !p2.ready() );
      BOOST_CHECK( !p3.ready() );

      waiter->set_value(); // signal p1.f1 to continue

      p3.wait(); // and wait for p3.f2 to complete

      BOOST_CHECK( p1.ready() );
      BOOST_CHECK( p2.ready() );
      BOOST_CHECK( p3.ready() );
      BOOST_CHECK_EQUAL( 3u, counter.load() );
   }
}

BOOST_AUTO_TEST_SUITE_END()
