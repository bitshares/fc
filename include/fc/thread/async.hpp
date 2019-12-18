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

#pragma once

#include <fc/thread/fibers.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/promise.hpp>
#include <boost/fiber/future/packaged_task.hpp>

#include <memory>

namespace fc {

   /** Executes f asynchronously in thread dest
    *  @return a future for the result of f()
    */
   template<typename Functor>
   auto async( Functor&& f, std::thread::id dest = std::this_thread::get_id(),
               const std::string& name = std::string() )
         -> boost::fibers::future<decltype(f())> {
      typedef decltype(f()) Result;
      boost::fibers::packaged_task<Result()> task;
      if( name.empty() )
         task = boost::fibers::packaged_task<Result()>( std::move(f) );
      else
         task = boost::fibers::packaged_task<Result()>( [f=std::move(f),&name] () mutable {
            set_fiber_name( name );
            return f();
         });
      boost::fibers::future<Result> r = task.get_future();
      boost::fibers::fiber fib( std::move( task ) );
      if( dest != std::this_thread::get_id() )
         target_thread_scheduler_base::move_fiber( fib, dest );
      fib.detach();
      return r;
   }

   /** Schedules f for execution in thread dest at time t
    *  @return a future for the result of f()
    */
   template<typename Functor, typename Clock, typename Duration>
   auto schedule( Functor&& f, std::chrono::time_point< Clock, Duration > const& t,
                  std::thread::id dest = std::this_thread::get_id(), const std::string name = std::string() )
         -> boost::fibers::future<decltype(f())> {
      return async( [f=std::move(f), t] () {
         boost::this_fiber::sleep_until( t );
         return f();
      }, dest, name );
   }

}
