#include <boost/fiber/future/future.hpp>

#include <fc/thread/fibers.hpp>

namespace fc { namespace test {

class worker_thread
{
   std::thread _thread;
   boost::fibers::condition_variable _cv;
   boost::fibers::mutex _mtx;
   bool _shutdown = false;
   bool _ready = false;

   public:
      worker_thread()
      {
         _thread = std::thread( [this] () {
            initialize_fibers();
            std::unique_lock<boost::fibers::mutex> lock(_mtx);
            _ready = true;
            _cv.notify_all();
            _cv.wait( lock, [this] () { return _shutdown; } );
         });

         std::unique_lock<boost::fibers::mutex> lock(_mtx);
         _cv.wait( lock, [this] () { return _ready; } );
      }
      ~worker_thread()
      {
         std::unique_lock<boost::fibers::mutex> lock(_mtx);
         _shutdown = true;
         _cv.notify_all();
         lock.unlock();
         _thread.join();
      }
      std::thread::id id() { return _thread.get_id(); }
};

class sync_point
{
   bool is_set = false;
   boost::fibers::mutex mtx;
   boost::fibers::condition_variable cv;
public:
   void reset() { is_set = false; }
   void set() { is_set = true; cv.notify_all(); }
   void wait() {
      std::unique_lock<boost::fibers::mutex> lock(mtx);
      cv.wait( lock, [this] () { return is_set; } );
   }
};

}} // fc::test
