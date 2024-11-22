/* This source is adapted from https://github.com/kmicklas/variadic-variant .
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once

#include <array>
#include <functional>
#include <stdexcept>
#include <typeinfo>

#include <fc/exception/exception.hpp>

namespace fc {

// Implementation details, the user should not import this:
namespace impl {

class dynamic_storage
{
    char* storage;
public:
    dynamic_storage();

    ~dynamic_storage();

    void* data() const;

    void alloc( size_t size );

    void release();
};

} // namespace impl

template<typename... Types>
class static_variant {
public:
    using tag_type = int64_t;
    using list = typelist::list<Types...>;

protected:
    static_assert(typelist::length<typelist::filter<list, std::is_reference>>() == 0,
                  "Reference types are not permitted in static_variant.");
    static_assert(typelist::length<typelist::concat_unique<list>>() == typelist::length<list>(),
                  "static_variant type arguments contain duplicate types.");

    template<typename X>
    using type_in_typelist = std::enable_if_t<typelist::index_of<list, X>() != -1>;

    tag_type _tag;
    impl::dynamic_storage storage;

    template<typename X, typename = type_in_typelist<X>>
    void init(const X& x) {
        _tag = typelist::index_of<list, X>();
        storage.alloc( sizeof(X) );
        new(storage.data()) X(x);
    }

    template<typename X, typename = type_in_typelist<X>>
    void init(X&& x) {
        _tag = typelist::index_of<list, X>();
        storage.alloc( sizeof(X) );
        new(storage.data()) X( std::move(x) );
    }

    void init_from_tag(tag_type tag)
    {
        FC_ASSERT( tag >= 0, "Unable to init with a negative tag '${tag}'", ("tag",tag) );
        FC_ASSERT( static_cast<size_t>(tag) < count(),
                   "Unable to init with tag '${tag}' when the number of supported tags is ${count}",
                   ("tag",tag) ("count",count()) );
        _tag = tag;
        typelist::runtime::dispatch(list(), tag, [this](auto t) {
            using T = typename decltype(t)::type;
            storage.alloc(sizeof(T));
            new(reinterpret_cast<T*>(storage.data())) T();
        });
    }

    void clean()
    {
        typelist::runtime::dispatch(list(), _tag, [data=storage.data()](auto t) {
            using T = typename decltype(t)::type;
            reinterpret_cast<T*>(data)->~T();
        });
        storage.release();
    }

    template<typename T, typename = void>
    struct import_helper {
        static static_variant construct(const T&) {
            FC_THROW_EXCEPTION(assert_exception, "Cannot import unsupported type ${T} into static_variant",
                               ("T", get_typename<T>::name()));
        }
        static static_variant construct(T&&) {
            FC_THROW_EXCEPTION(assert_exception, "Cannot import unsupported type ${T} into static_variant",
                               ("T", get_typename<T>::name()));
        }
    };
    template<typename T>
    struct import_helper<T, type_in_typelist<T>> {
        static static_variant construct(const T& t) {
            return static_variant(t);
        }
        static static_variant construct(T&& t) {
            return static_variant(std::move(t));
        }
    };

public:
    template<typename X, typename = type_in_typelist<X>>
    struct tag
    {
       static constexpr tag_type value = typelist::index_of<list, X>();
    };

    struct type_lt {
       bool operator()(const static_variant& a, const static_variant& b) const { return a.which() < b.which(); }
    };
    struct type_eq {
       bool operator()(const static_variant& a, const static_variant& b) const { return a.which() == b.which(); }
    };
    using flat_set_type = flat_set<static_variant, type_lt>;

    /// Import the value from a foreign static_variant with types not in this one, and throw if the value is an
    /// incompatible type
    template<typename... Other>
    static static_variant import_from(const static_variant<Other...>& other) {
        return typelist::runtime::dispatch(typelist::list<Other...>(), other.which(), [&other](auto t) {
            using other_type = typename decltype(t)::type;
            return import_helper<other_type>::construct(other.template get<other_type>());
        });
    }
    /// Import the value from a foreign static_variant with types not in this one, and throw if the value is an
    /// incompatible type
    template<typename... Other>
    static static_variant import_from(static_variant<Other...>&& other) {
        return typelist::runtime::dispatch(typelist::list<Other...>(), other.which(), [&other](auto t) {
            using other_type = typename decltype(t)::type;
            return import_helper<other_type>::construct(std::move(other.template get<other_type>()));
        });
    }

    static_variant()
    {
        init_from_tag(0);
    }

    template<typename... Other>
    static_variant( const static_variant<Other...>& cpy )
    {
       typelist::runtime::dispatch(typelist::list<Other...>(), cpy.which(), [this, &cpy](auto t) mutable {
          this->init(cpy.template get<typename decltype(t)::type>());
       });
    }

    static_variant( const static_variant& cpy )
    {
       typelist::runtime::dispatch(list(), cpy.which(), [this, &cpy](auto t) mutable {
          this->init(cpy.template get<typename decltype(t)::type>());
       });
    }

    static_variant( static_variant&& mv )
    {
       typelist::runtime::dispatch(list(), mv.which(), [this, &mv](auto t) mutable {
          this->init(std::move(mv.template get<typename decltype(t)::type>()));
       });
    }

    template<typename... Other>
    static_variant( static_variant<Other...>&& mv )
    {
       typelist::runtime::dispatch(typelist::list<Other...>(), mv.which(), [this, &mv](auto t) mutable {
           this->init(std::move(mv.template get<typename decltype(t)::type>()));
       });
    }

    template<typename X, typename = type_in_typelist<X>>
    static_variant(const X& v) {
        init(v);
    }
    template<typename X, typename = type_in_typelist<X>>
    static_variant(X&& v) {
        init(std::move(v));
    }

    ~static_variant() {
       clean();
    }

    template<typename X, typename = type_in_typelist<X>>
    static_variant& operator=(const X& v) {
        clean();
        init(v);
        return *this;
    }
    static_variant& operator=( const static_variant& v )
    {
       if( this == &v ) return *this;
       clean();
       typelist::runtime::dispatch(list(), v.which(), [this, &v](auto t)mutable {
          this->init(v.template get<typename decltype(t)::type>());
       });
       return *this;
    }
    static_variant& operator=( static_variant&& v )
    {
       if( this == &v ) return *this;
       clean();
       typelist::runtime::dispatch(list(), v.which(), [this, &v](auto t)mutable {
          this->init(std::move(v.template get<typename decltype(t)::type>()));
       });
       return *this;
    }

    friend bool operator==( const static_variant& a, const static_variant& b ) {
       if (a.which() != b.which())
          return false;
       return typelist::runtime::dispatch(list(), a.which(), [&a, &b](auto t) {
          using Value = typename decltype(t)::type;
          return a.template get<Value>() == b.template get<Value>();
       });
    }

    template<typename X, typename = type_in_typelist<X>>
    X& get() {
        if(_tag == typelist::index_of<list, X>()) {
            return *reinterpret_cast<X*>(storage.data());
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception,
                                "static_variant does not contain a value of type ${t}",
                                ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename X, typename = type_in_typelist<X>>
    const X& get() const {
        if(_tag == typelist::index_of<list, X>()) {
            return *reinterpret_cast<const X*>(storage.data());
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception,
                                "static_variant does not contain a value of type ${t}",
                                ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename visitor>
    typename visitor::result_type visit(visitor& v) {
        return visit( _tag, v, (void*) storage.data() );
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v) {
        return visit( _tag, v, (void*) storage.data() );
    }

    template<typename visitor>
    typename visitor::result_type visit(visitor& v)const {
        return visit( _tag, v, (const void*) storage.data() );
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v)const {
        return visit( _tag, v, (const void*) storage.data() );
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, visitor& v, void* data )
    {
        FC_ASSERT( tag >= 0 && static_cast<size_t>(tag) < count(),
                   "Unsupported type '${tag}', the number of supported types is ${count}! ",
                   ("tag",tag) ("count",count()) );
        return typelist::runtime::dispatch(list(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, const visitor& v, void* data )
    {
        FC_ASSERT( tag >= 0 && static_cast<size_t>(tag) < count(),
                   "Unsupported type '${tag}', the number of supported types is ${count}! ",
                   ("tag",tag) ("count",count()) );
        return typelist::runtime::dispatch(list(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, visitor& v, const void* data )
    {
        FC_ASSERT( tag >= 0 && static_cast<size_t>(tag) < count(),
                   "Unsupported type '${tag}', the number of supported types is ${count}! ",
                   ("tag",tag) ("count",count()) );
        return typelist::runtime::dispatch(list(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<const typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, const visitor& v, const void* data )
    {
        FC_ASSERT( tag >= 0 && static_cast<size_t>(tag) < count(),
                   "Unsupported type '${tag}', the number of supported types is ${count}! ",
                   ("tag",tag) ("count",count()) );
        return typelist::runtime::dispatch(list(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<const typename decltype(t)::type*>(data));
        });
    }

    static constexpr size_t count() { return typelist::length<list>(); }
    void set_which( tag_type tag ) {
      FC_ASSERT( tag >= 0, "Unable to set a negative tag '${tag}'", ("tag",tag) );
      FC_ASSERT( static_cast<size_t>(tag) < count(),
                 "Unable to set tag '${tag}' when the number of supported tags is ${count}",
                 ("tag",tag) ("count",count()) );
      clean();
      init_from_tag(tag);
    }

    tag_type which() const {return _tag;}

    template<typename T>
    bool is_type() const { return _tag == tag<T>::value; }
};
template<> class static_variant<> {
public:
    using tag_type = int64_t;
    static_variant() { FC_THROW_EXCEPTION(assert_exception, "Cannot create static_variant with no types"); }
};
template<typename... Types> class static_variant<typelist::list<Types...>> : public static_variant<Types...> {};

   struct from_static_variant
   {
      variant& var;
      const uint32_t _max_depth;
      from_static_variant( variant& dv, uint32_t max_depth ):var(dv),_max_depth(max_depth){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         to_variant( v, var, _max_depth );
      }
   };

   struct to_static_variant
   {
      const variant& var;
      const uint32_t _max_depth;
      to_static_variant( const variant& dv, uint32_t max_depth ):var(dv),_max_depth(max_depth){}

      typedef void result_type;
      template<typename T> void operator()( T& v )const
      {
         from_variant( var, v, _max_depth );
      }
   };


   template<typename... T> void to_variant( const fc::static_variant<T...>& s, fc::variant& v, uint32_t max_depth )
   {
      FC_ASSERT( max_depth > 0 );
      variants vars(2);
      vars[0] = s.which();
      s.visit( from_static_variant(vars[1], max_depth - 1) );
      v = std::move(vars);
   }
   template<typename... T> void from_variant( const fc::variant& v, fc::static_variant<T...>& s, uint32_t max_depth )
   {
      FC_ASSERT( max_depth > 0 );
      auto ar = v.get_array();
      if( ar.size() < 2 ) return;
      s.set_which( ar[0].as_uint64() );
      s.visit( to_static_variant(ar[1], max_depth - 1) );
   }

   template< typename... T > struct get_comma_separated_typenames;

   template<>
   struct get_comma_separated_typenames<>
   {
      static const char* names() { return ""; }
   };

   template< typename T >
   struct get_comma_separated_typenames<T>
   {
      static const char* names()
      {
         static const std::string n = get_typename<T>::name();
         return n.c_str();
      }
   };

   template< typename T, typename... Ts >
   struct get_comma_separated_typenames<T, Ts...>
   {
      static const char* names()
      {
         static const std::string n =
            std::string( get_typename<T>::name() )+","+
            std::string( get_comma_separated_typenames< Ts... >::names() );
         return n.c_str();
      }
   };

   template< typename... T >
   struct get_typename< static_variant< T... > >
   {
      static const char* name()
      {
         static const std::string n = std::string( "fc::static_variant<" )
            + get_comma_separated_typenames<T...>::names()
            + ">";
         return n.c_str();
      }
   };

} // namespace fc
