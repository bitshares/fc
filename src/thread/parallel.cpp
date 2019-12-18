/*
 * Copyright (c) 2018-2019 The BitShares Blockchain, and contributors.
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

#include <fc/asio.hpp>
#include <fc/exception/exception.hpp>
#include <fc/thread/fibers.hpp>
#include <fc/thread/parallel.hpp>

#include <boost/fiber/algo/round_robin.hpp>
#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <queue>

namespace fc {
   namespace detail {

      class pool_impl;

      class pool_scheduler : public boost::fibers::algo::algorithm
      {
      public:
         pool_scheduler( pool_impl& pool ) : _pool( pool ) {}
         virtual ~pool_scheduler() = default;

         virtual void awakened( boost::fibers::context* ctx ) noexcept;
         virtual boost::fibers::context* pick_next() noexcept;
         virtual bool has_ready_fibers() const noexcept;
         virtual void suspend_until( const std::chrono::steady_clock::time_point& then ) noexcept;
         virtual void notify() noexcept;

      private:
         pool_impl&              _pool;
         std::queue<boost::fibers::context*> pinned;
      };

      class pool_impl
      {
      public:
         explicit pool_impl( const uint16_t num_threads )
         {
            FC_ASSERT( num_threads > 0, "A worker pool should have at least one thread!" );
            threads.reserve( num_threads );
            std::atomic<uint32_t> ready{ 0 };
            for( uint32_t i = 0; i < num_threads; i++ )
            {
               threads.emplace_back( [this,&ready,i] () {
                  set_thread_name( "pool worker #" + fc::to_string(i) );
                  boost::fibers::use_scheduling_algorithm< target_thread_scheduler< pool_scheduler > >( *this );
                  std::unique_lock< boost::fibers::mutex > lock( close_wait_mutex );
                  ready++;
                  close_wait.notify_all();
                  lock.unlock();
                  while( !closing )
                     boost::this_fiber::sleep_for( std::chrono::seconds(3) );
               } );
            }
            std::unique_lock< boost::fibers::mutex > lock( close_wait_mutex );
            close_wait.wait( lock, [this,&ready,num_threads] () { return ready.load() == num_threads; } );
         }
         pool_impl( pool_impl& copy ) = delete;
         pool_impl( pool_impl&& move ) = delete;
         ~pool_impl() 
         {
            closing = true;
            for( std::thread& thread : threads ) thread.join();
         }

         void post( boost::fibers::fiber&& fiber )
         {
            target_thread_scheduler_base::move_fiber( fiber, threads[0].get_id() );
            fiber.detach();
         }

      private:
         std::vector<std::thread>          threads;
         bool                              closing = false;
         boost::fibers::condition_variable close_wait;
         boost::fibers::mutex              close_wait_mutex;

         boost::lockfree::queue<boost::fibers::context*> ready_queue{200};
         std::condition_variable suspender;
         std::mutex              suspend_mutex;

         friend class pool_scheduler;
      };


      void pool_scheduler::awakened( boost::fibers::context* ctx ) noexcept
      {
         if( ctx->is_context( boost::fibers::type::pinned_context ) )
            pinned.push( ctx );
         else
         {
            ctx->detach();
            _pool.ready_queue.push( ctx );
            _pool.suspender.notify_one();
         }
      }

      boost::fibers::context* pool_scheduler::pick_next() noexcept
      {
         boost::fibers::context* result = nullptr;
         if( !_pool.ready_queue.pop( result ) )
         {
            if( !pinned.empty() )
            {
               result = pinned.front();
               pinned.pop();
            }
         }
         else
            boost::fibers::context::active()->attach( result );
         return result;
      }

      bool pool_scheduler::has_ready_fibers() const noexcept
      {
         return !_pool.ready_queue.empty() || !pinned.empty();
      }

      void pool_scheduler::suspend_until( const std::chrono::steady_clock::time_point& then ) noexcept
      {
         std::unique_lock< std::mutex > lock( _pool.suspend_mutex );
         if( then == std::chrono::steady_clock::time_point::max() )
            _pool.suspender.wait( lock );
         else
            _pool.suspender.wait_until( lock, then );
      }

      void pool_scheduler::notify() noexcept
      {
         _pool.suspender.notify_all();
      }


      worker_pool::worker_pool()
      {
         fc::asio::default_io_service();
         my = std::make_unique<pool_impl>( fc::asio::default_io_service_scope::get_num_threads() );
      }

      worker_pool::~worker_pool() {}

      void worker_pool::post( boost::fibers::fiber&& task )
      {
         my->post( std::move( task ) );
      }

      worker_pool& get_worker_pool()
      {
         static worker_pool the_pool;
         // Note: the destructor of pool_impl throws in all_tests because boost::fibers::mutex detects a deadlock.
         // I think the problem is not a deadlock but that the destructor is run from a thread that has no
         // thread_scheduler defined.
         return the_pool;
      }
   }

   serial_valve::ticket_guard::ticket_guard( std::shared_ptr<boost::fibers::promise<void>>& latch )
   {
      my_promise = std::make_shared<boost::fibers::promise<void>>();
      std::shared_ptr<boost::fibers::promise<void>> tmp;
      do
      {
         tmp = std::atomic_load( &latch );
         FC_ASSERT( tmp, "Valve is shutting down!" );
      }
      while( !std::atomic_compare_exchange_weak( &latch, &tmp, my_promise ) );
      ticket = tmp->get_future();
   }

   serial_valve::ticket_guard::~ticket_guard()
   {
      my_promise->set_value();
   }

   void serial_valve::ticket_guard::wait_for_my_turn()
   {
       ticket.wait();
   }

   serial_valve::serial_valve()
   {
      auto start_promise = std::make_shared<boost::fibers::promise<void>>();
      std::atomic_store( &latch, start_promise );
      start_promise->set_value();
   }

   serial_valve::~serial_valve()
   {
      auto last = std::atomic_exchange( &latch, std::shared_ptr<boost::fibers::promise<void>>() );
      if( last )
         last->get_future().wait();
   }
} // namespace fc
