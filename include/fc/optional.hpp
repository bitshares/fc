#pragma once
#include <assert.h>
#include <utility>

namespace fc {
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4521)  /* multiple copy ctors */
# pragma warning(disable:4522) /* multiple assignment operators */
#endif
  bool assert_optional(bool is_valid ); // defined in exception.cpp

  /**
   *  @brief provides stack-based nullable value similar to boost::optional
   *
   *  Simply including boost::optional adds 35,000 lines to each object file, using
   *  fc::optional adds less than 400.
   */
  template<typename T>
  class optional
  {
    public:
      using value_type = T;

      optional() = default;
      ~optional(){ reset(); }

      optional( const optional& o )
      : _valid( o._valid )
      {
        if( o._valid ) new (ptr()) T( *o );
      }

      optional( optional&& o )
      : _valid( o._valid )
      {
        if( o._valid ) new (ptr()) T( std::move(*o) );
      }

      template<typename U>
      explicit optional( const optional<U>& o )
      : _valid( o._valid )
      {
        if( o._valid ) new (ptr()) T( *o );
      }

      template<typename U>
      explicit optional( optional<U>&& o )
      : _valid( o._valid )
      {
        if( o._valid ) new (ptr()) T( std::move(*o) );
      }

      template<typename U>
      explicit optional( U&& u )
      :_valid(true)
      {
        new ((char*)ptr()) T( std::forward<U>(u) );
      }

      optional& operator=( const optional& o )
      {
        if (this != &o) {
          if( _valid && o._valid ) {
            ref() = *o;
          } else if( !_valid && o._valid ) {
             new (ptr()) T( *o );
             _valid = true;
          } else if (_valid) {
            reset();
          }
        }
        return *this;
      }

      optional& operator=( optional&& o )
      {
        if (this != &o)
        {
          if( _valid && o._valid )
          {
            ref() = std::move(*o);
          } else if ( !_valid && o._valid ) {
            *this = std::move(*o);
          } else if (_valid) {
            reset();
          }
        }
        return *this;
      }

      template<typename U>
      optional& operator=( const optional<U>& o )
      {
        if (this != &o) {
          if( _valid && o._valid ) {
            ref() = *o;
          } else if( !_valid && o._valid ) {
             new (ptr()) T( *o );
             _valid = true;
          } else if (_valid) {
            reset();
          }
        }
        return *this;
      }

      template<typename U>
      optional& operator=( optional<U>&& o )
      {
        if (this != &o)
        {
          if( _valid && o._valid )
          {
            ref() = std::move(*o);
          } else if ( !_valid && o._valid ) {
            *this = std::move(*o);
          } else if (_valid) {
            reset();
          }
        }
        return *this;
      }

      template<typename U>
      optional& operator=( U&& u )
      {
        reset();
        new (ptr()) T( std::forward<U>(u) );
        _valid = true;
        return *this;
      }

      bool valid()const     { return _valid;  }
      bool operator!()const { return !_valid; }

      // this operation is not safe and can result in unintential
      // casts and comparisons, use valid() or !!
      explicit operator bool()const  { return _valid;  }

      T&       operator*()      { assert(_valid); return ref(); }
      const T& operator*()const { assert(_valid); return ref(); }

      T*       operator->()
      {
         assert(_valid);
         return ptr();
      }
      const T* operator->()const
      {
         assert(_valid);
         return ptr();
      }

      optional& operator=(std::nullptr_t)
      {
        reset();
        return *this;
      }

      friend bool operator < ( const optional& a, const optional& b )
      {
         if( a.valid() && b.valid() ) return *a < *b;
         return a.valid() < b.valid();
      }
      friend bool operator == ( const optional& a, const optional& b )
      {
         if( a.valid() && b.valid() ) return *a == *b;
         return a.valid() == b.valid();
      }
      friend bool operator != ( const optional& a, const optional& b )
      {
         return !( a == b );
      }

      void     reset()
      {
          if( _valid )
          {
              ref().~T(); // call destructor
          }
          _valid = false;
      }
    private:
      template<typename U> friend class optional;
      T&       ref()      { return *ptr(); }
      const T& ref()const { return *ptr(); }
      T*       ptr()      { void* v = &_value[0]; return static_cast<T*>(v); }
      const T* ptr()const { const void* v = &_value[0]; return static_cast<const T*>(v); }

      // force alignment... to 16 byte boundaries
#if defined(_MSC_VER)
      double _value[((sizeof(T)+7)/8)] __declspec((align(16)));
#else
      double _value[((sizeof(T)+7)/8)] __attribute__((aligned(16)));
#endif
      bool   _valid = false;
  };

  template<typename T, typename U>
  bool operator == ( const optional<T>& left, const U& u ) {
    return !!left && *left == u;
  }
  template<typename T, typename U>
  bool operator != ( const optional<T>& left, const U& u ) {
    return !left || *left != u;
  }

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // namespace fc

