/*
 * Copyright (c) 2019 The BitShares Blockchain, and contributors.
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

#pragma once

#include <boost/fiber/algo/algorithm.hpp>
#include <boost/fiber/fiber.hpp>
#include <boost/lockfree/queue.hpp>

#include <memory>
#include <thread>

namespace fc {

   class target_thread_scheduler_base : public boost::fibers::algo::algorithm
   {
   public:
      target_thread_scheduler_base();
      virtual ~target_thread_scheduler_base();

      virtual void awakened( boost::fibers::context* ctx ) noexcept;
      virtual boost::fibers::context* pick_next() noexcept;
      virtual bool has_ready_fibers() const noexcept;
      virtual void suspend_until( const std::chrono::steady_clock::time_point& then ) noexcept;
      virtual void notify() noexcept;

      void add_fiber( boost::fibers::context* ctx )noexcept;

      static void move_fiber( const boost::fibers::fiber& fiber, const std::thread::id dest );

   private:
      virtual boost::fibers::algo::algorithm& get_delegate()noexcept = 0;
      virtual const boost::fibers::algo::algorithm& get_delegate()const noexcept = 0;
      void requeue()noexcept;

      boost::lockfree::queue<boost::fibers::context*> ready_queue{200};
   };
      
   /** In order to support boost fibers, a thread must specify a scheduling algorithm.
    *  The scheduling algorithm manages fibers that are ready to run, and selects the
    *  next fiber when a context switch is about to happen.
    *
    *  Boost::fiber comes with a several useful algorithms. However, none of these
    *  support executing a fiber in a different thread. For this, we have to roll our
    *  own scheduler.
    *
    *  In order to launch fibers in other threads *OR* to run fibers created by other
    *  threads, a thread must use an instantiation of target_thread_scheduler.
    *
    *  target_thread_scheduler only handles the actual migration of fibers between
    *  threads, while the "normal" scheduling work is done by a parent scheduler.
    */
   template< typename P >
   class target_thread_scheduler : public target_thread_scheduler_base
   {
   public:
      target_thread_scheduler() {}
      template< typename ... Args >
      target_thread_scheduler( Args&& ... args ) : parent( args ... ) {}
      virtual ~target_thread_scheduler() {}

   private:
      virtual boost::fibers::algo::algorithm& get_delegate()noexcept { return parent; }
      virtual const boost::fibers::algo::algorithm& get_delegate()const noexcept { return parent; }

      P parent;
   };

   /** A thread that wants to be able to work on fibers delegated by some other thread must call this exactly
    *  once.
    */
   void initialize_fibers();

   const std::string& get_thread_name();
   void set_thread_name( const std::string& name );

   const std::string& get_fiber_name();
   void set_fiber_name( const std::string& name );
} // fc
