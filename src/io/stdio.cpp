#include <fc/exception/exception.hpp>
#include <fc/io/sstream.hpp>
#include <fc/io/stdio.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/fibers.hpp>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace fc {
  
  class cin_buffer {
  public:
    cin_buffer() : eof(false), write_pos(0), read_pos(0) {
       cinthread = std::thread( [this] () {
          set_thread_name("cin");
          read();
       } );
    }

    ~cin_buffer()
    {
       {
          std::unique_lock<std::mutex> lock( mtx );
          eof = true;
          read_ready.notify_all();
          write_ready.notify_all();
       }
       cinthread.join();
    }

    size_t readsome( char* dest, size_t len ) {
      std::unique_lock<std::mutex> lock( mtx );
      return _readsome( dest, len );
    }

    size_t read( char* dest, size_t len ) {
      std::unique_lock<std::mutex> lock( mtx );
      if( eof ) FC_THROW_EXCEPTION( eof_exception, "cin" );

      size_t total = 0;
      while( len > 0 ) { 
         size_t done = _readsome( dest, len );
         total += done;
         dest += done;
         len -= done;
         if( len == 0 ) break;
         if( eof ) FC_THROW_EXCEPTION( eof_exception, "cin" );
         read_ready.wait( lock );
      }
      return total;
    }

    bool is_eof() { return eof; }

  private:
    void read() {
      char c;
      std::cin.read(&c,1);
      std::unique_lock<std::mutex> lock( mtx );
      while( !std::cin.eof() && !eof ) {
        write_ready.wait( lock, [this] () { return eof || write_pos - read_pos <= 0xfffff; } );
        if( write_pos - read_pos <= 0xfffff )
        {
           buf[write_pos&0xfffff] = c;
           ++write_pos;
        }
        read_ready.notify_all();
        lock.unlock();
        std::cin.read(&c,1);
        lock.lock();
      }
      eof = true;
      read_ready.notify_all();
    }

    /* This must be only called while holding a lock on mtx! */
    size_t _readsome( char* dest, size_t len ) {
      uint64_t avail = write_pos - read_pos;
      if( len < avail ) len = avail;

      size_t u = 0;
      while( len > 0 ) {
         *dest++ = buf[(read_pos++)&0xfffff]; 
         --len;
         ++u;
      }
      return u;
    }

    std::mutex              mtx;
    std::condition_variable read_ready;
    std::condition_variable write_ready;
    
    bool     eof;
    uint64_t write_pos;
    char     buf[0xfffff+1]; // 1 mb buffer
    uint64_t read_pos;

    std::thread cinthread;
  };

  cin_buffer& get_cin_buffer() {
    static cin_buffer b;
    return b;
  }

  size_t cout_t::writesome( const char* buf, size_t len )
  {
     std::cout.write(buf,len);
     return len;
  }
  size_t cout_t::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
  {
     return writesome(buf.get() + offset, len);
  }
  void   cout_t::close() {}
  void   cout_t::flush() { std::cout.flush(); }

  size_t cerr_t::writesome( const char* buf, size_t len )
  {
     std::cerr.write(buf,len);
     return len;
  }
  size_t cerr_t::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
  {
     return writesome(buf.get() + offset, len);
  }
  void   cerr_t::close() {};
  void   cerr_t::flush() { std::cerr.flush(); }

  size_t cin_t::readsome( char* buf, size_t len ) {
    cin_buffer& b = get_cin_buffer();
    size_t u = b.readsome( buf, len );
    if( u == 0 && len > 0 )
    {
       b.read( buf, 1 );
       if( len > 1 )
          u = 1 + b.readsome( buf + 1, len - 1 );
    }
    return u;
  }

  size_t cin_t::readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset )
  {
     return readsome( buf.get() + offset, len );
  }

  istream& cin_t::read( char* buf, size_t len ) {
    cin_buffer& b = get_cin_buffer();
    b.read( buf, len );
    return *this;
  }

  bool cin_t::eof()const { return get_cin_buffer().is_eof(); }
  
  std::shared_ptr<cin_t>  cin_ptr = std::make_shared<cin_t>();
  std::shared_ptr<cout_t> cout_ptr = std::make_shared<cout_t>();
  std::shared_ptr<cerr_t> cerr_ptr = std::make_shared<cerr_t>();
  cout_t& cout = *cout_ptr;
  cerr_t& cerr = *cerr_ptr;
  cin_t&  cin  = *cin_ptr;

} // namespace fc
