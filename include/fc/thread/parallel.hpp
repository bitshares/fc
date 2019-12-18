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

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/promise.hpp>
#include <boost/fiber/future/packaged_task.hpp>
#include <boost/fiber/fiber.hpp>

#include <memory>

namespace fc {

   namespace detail {
      class pool_impl;

      class worker_pool {
      public:
         worker_pool();
         ~worker_pool();
         template<typename T>
         void post( boost::fibers::packaged_task< T >&& task )
         {
            post( boost::fibers::fiber( std::move( task ) ) );
         }
         void post( boost::fibers::fiber&& fiber );
      private:
          std::unique_ptr<pool_impl> my;
      };

      worker_pool& get_worker_pool();
   }

   class serial_valve {
   private:
      class ticket_guard {
      public:
         explicit ticket_guard( std::shared_ptr<boost::fibers::promise<void>>& latch );
         ~ticket_guard();
         void wait_for_my_turn();
      private:
         std::shared_ptr<boost::fibers::promise<void>> my_promise;
         boost::fibers::future<void> ticket;
      };

      friend class ticket_guard;
      std::shared_ptr<boost::fibers::promise<void>> latch;

   public:
      serial_valve();
      ~serial_valve();

      /** Executes f1() then f2().
       *  For any two calls do_serial(f1,f2) and do_serial(f1',f2') where
       *  do_serial(f1,f2) is invoked before do_serial(f1',f2'), it is
       *  guaranteed that f2' will be executed after f2 has completed. Failure
       *  of either function counts as completion of both.
       *  If f1 throws then f2 will not be invoked.
       *
       * @param f1 a functor to invoke
       * @param f2 a functor to invoke
       * @return the return value of f2()
       */
      template<typename Functor1,typename Functor2>
      auto do_serial( const Functor1& f1, const Functor2& f2 ) -> decltype(f2())
      {
         ticket_guard guard( latch );
         f1();
         guard.wait_for_my_turn();
         return f2();
      }
   };

   /**
    *  Calls function <code>f</code> in a separate thread and returns a future
    *  that can be used to wait on the result.
    *
    *  @param f the operation to perform
    */
   template<typename Functor>
   auto do_parallel( Functor&& f ) -> boost::fibers::future<decltype(f())> {
      typedef decltype(f()) Result;
      boost::fibers::packaged_task<Result()> task( std::move(f) );
      boost::fibers::future<Result> r = task.get_future();
      detail::get_worker_pool().post( std::move(task) );
      return r;
   }
}
