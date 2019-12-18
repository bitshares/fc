#pragma once 
#include <functional>

#include <boost/any.hpp>
#include <boost/config.hpp>

#include <fc/exception/exception.hpp>
#include <fc/optional.hpp>

// ms visual c++ (as of 2013) doesn't accept the standard syntax for calling a 
// templated member function (foo->template bar();)
#ifdef _MSC_VER
# define FC_CALL_MEMBER_TEMPLATE_KEYWORD
#else
# define FC_CALL_MEMBER_TEMPLATE_KEYWORD template
#endif

namespace fc {
  namespace detail {
    /// This metafunction determines whether its template argument is an instantiation of fc::optional
    template<typename T> struct is_optional : public std::false_type {};
    template<typename T> struct is_optional<fc::optional<T>> : public std::true_type {};
    /// This metafunction determines whether all of its template arguments are instantiations of fc::optional
    template<typename... Ts> struct all_optionals;
    template<> struct all_optionals<> : public std::true_type {};
    template<typename T, typename... Ts> struct all_optionals<T, Ts...> : public std::false_type {};
    template<typename T, typename... Ts> struct all_optionals<fc::optional<T>, Ts...> : public all_optionals<Ts...> {};

    /// A wrapper of std::function allowing callers to omit the last several arguments if those arguments are
    /// fc::optional types. i.e. given a function taking (int, double, bool, fc::optional<string>, fc::optional<char>),
    /// whereas normally the last two arguments must be provided, this template allows them to be omitted.
    /// Note that this only applies to trailing optional arguments, i.e. given a callable taking
    /// (fc::optional<int>, int, fc::optional<int>), only the last argument can be omitted.
    ///
    /// A discussion of how exactly this works is available here:
    /// https://github.com/bitshares/bitshares-fc/pull/126#issuecomment-490566060
    template<typename R, typename... Parameters>
    struct optionals_callable : public std::function<R(Parameters...)> {
       using std::function<R(Parameters...)>::operator();

       template<typename... CutList>
       struct short_pack {};
       /// This metafunction removes the first several types from a variadic parameter pack of types.
       /// The first parameter is the number of arguments to remove from the beginning of the pack.
       /// All subsequent parameters are types in the list to be cut
       /// The result pack_cutter<...>::type is a short_pack<RemainingTypes>
       template<unsigned RemoveCount, typename... Types>
       struct pack_cutter;
       template<unsigned RemoveCount, typename, typename... Types>
       struct pack_cutter_impl;
       template<typename... Types>
       struct pack_cutter_impl<0, void, Types...> {
           static_assert(all_optionals<Types...>::value, "All omitted arguments must correspond to optional parameters.");
           using type = short_pack<Types...>;
       };
       template<unsigned RemoveCount, typename T, typename... Types>
       struct pack_cutter_impl<RemoveCount, std::enable_if_t<RemoveCount != 0>, T, Types...>
               : public pack_cutter_impl<RemoveCount - 1, void, Types...> {};
       template<unsigned RemoveCount, typename... Types>
       struct pack_cutter : public pack_cutter_impl<RemoveCount, void, Types...> {};
       template<unsigned RemoveCount, typename... Types>
       using pack_cutter_t = typename pack_cutter<RemoveCount, Types...>::type;

       template<typename F, typename... OptionalTypes>
       R call_function(F&& f, short_pack<OptionalTypes...>) {
           return f(OptionalTypes()...);
       }

       /// Overload the function call operator, enabled if the caller provides fewer arguments than there are parameters.
       /// Pads out the provided arguments with default-constructed optionals, checking that they are indeed optional types
       template<class... Args>
       std::enable_if_t<sizeof...(Args) < sizeof...(Parameters), R> operator()(Args... args) {
           // Partially apply with the arguments provided
           auto partial_function = [this, &args...](auto&&... rest) {
               return (*this)(std::forward<decltype(args)>(args)..., std::move(rest)...);
           };
           // Cut the provided arguments' types out of the Parameters list, and store the rest in a dummy type
           pack_cutter_t<sizeof...(Args), std::decay_t<Parameters>...> dummy;
           // Pass the partially applied function and the dummy type to another function which can deduce the optional
           // types and call the function with default instantiations of those types
           return call_function(std::move(partial_function), dummy);
       }
    };
  }

  // This is no longer used and probably no longer can be used without generalizing the infrastructure around it, but I
  // kept it because it is informative.
//  struct identity_member {
//       template<typename R, typename C, typename P, typename... Args>
//       static std::function<R(Args...)> functor( P&& p, R (C::*mem_func)(Args...) );
//       template<typename R, typename C, typename P, typename... Args>
//       static std::function<R(Args...)> functor( P&& p, R (C::*mem_func)(Args...)const );
//  };
  /// Used as the Transform template parameter for APIs, this type has two main purposes: first, it reads the argument
  /// list and return type of a method into template parameters; and second, it uses those types in conjunction with the
  /// optionals_callable template above to create a function pointer which supports optional arguments.
  struct identity_member_with_optionals {
       template<typename R, typename C, typename P, typename... Args>
       static detail::optionals_callable<R, Args...> functor( P&& p, R (C::*mem_func)(Args...) );
       template<typename R, typename C, typename P, typename... Args>
       static detail::optionals_callable<R, Args...> functor( P&& p, R (C::*mem_func)(Args...)const );
  };

  template< typename Interface, typename Transform  >
  struct vtable  : public std::enable_shared_from_this<vtable<Interface,Transform>> 
  { private: vtable(); };
  
  template<typename OtherType>
  struct vtable_copy_visitor {
      typedef OtherType other_type;

      vtable_copy_visitor( OtherType& s):_source( s ){}
  
      template<typename R, typename MemberPtr, typename... Args>
      void operator()( const char* name, std::function<R(Args...)>& memb, MemberPtr m )const {
        OtherType* src = &_source;
        memb = [src,m]( Args... args ){ return (src->*m)(args...); };
      }
      OtherType& _source;
  };

  template<typename Interface, typename Transform >
  class api;

  class api_connection;

  typedef uint32_t api_id_type;

  class api_base
  {
     public:
        api_base() {}
        virtual ~api_base() {}

        virtual uint64_t get_handle()const = 0;

        virtual api_id_type register_api( api_connection& conn )const = 0;

        // defined in api_connection.hpp
        template< typename T >
        api<T, identity_member_with_optionals> as();
  };
  typedef std::shared_ptr< api_base > api_ptr;

  class api_connection;

  template<typename Interface, typename Transform = identity_member_with_optionals >
  class api : public api_base {
    public:
      typedef vtable<Interface,Transform> vtable_type;

      api():_vtable( std::make_shared<vtable_type>() ) {}

      /** T is anything with pointer semantics */
      template<typename T >
      api( const T& p )
      :_vtable( std::make_shared<vtable_type>() )
      {
         _data = std::make_shared<boost::any>(p);
         T& ptr = boost::any_cast<T&>(*_data);
         auto& pointed_at = *ptr;
         typedef typename std::remove_reference<decltype(pointed_at)>::type source_vtable_type;
         _vtable->FC_CALL_MEMBER_TEMPLATE_KEYWORD visit_other( vtable_copy_visitor<source_vtable_type>(pointed_at) );
      }

      api( const api& cpy ):_vtable(cpy._vtable),_data(cpy._data) {}
      virtual ~api() {}

      friend bool operator == ( const api& a, const api& b ) { return a._data == b._data && a._vtable == b._vtable;    }
      friend bool operator != ( const api& a, const api& b ) { return !(a._data == b._data && a._vtable == b._vtable); }
      virtual uint64_t get_handle()const override { return uint64_t(_data.get()); }
      virtual api_id_type register_api( api_connection& conn )const override;    // defined in api_connection.hpp

      vtable_type& operator*()const  { FC_ASSERT( _vtable ); return *_vtable; }
      vtable_type* operator->()const {  FC_ASSERT( _vtable ); return _vtable.get(); }

    protected:
      std::shared_ptr<vtable_type>    _vtable;
      std::shared_ptr<boost::any>     _data;
  };

} // namespace fc

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>

#define FC_API_VTABLE_DEFINE_MEMBER( r, data, elem ) \
      decltype(Transform::functor( (data*)nullptr, &data::elem)) elem; 
#define FC_API_VTABLE_DEFINE_VISIT_OTHER( r, data, elem ) \
        { typedef typename Visitor::other_type OtherType; \
        v( BOOST_PP_STRINGIZE(elem), elem, &OtherType::elem ); }
#define FC_API_VTABLE_DEFINE_VISIT( r, data, elem ) \
        v( BOOST_PP_STRINGIZE(elem), elem ); 

#define FC_API( CLASS, METHODS ) \
namespace fc { \
  template<typename Transform> \
  struct vtable<CLASS,Transform> : public std::enable_shared_from_this<vtable<CLASS,Transform>> { \
      BOOST_PP_SEQ_FOR_EACH( FC_API_VTABLE_DEFINE_MEMBER, CLASS, METHODS ) \
      template<typename Visitor> \
      void visit_other( Visitor&& v ){ \
        BOOST_PP_SEQ_FOR_EACH( FC_API_VTABLE_DEFINE_VISIT_OTHER, CLASS, METHODS ) \
      } \
      template<typename Visitor> \
      void visit( Visitor&& v ){ \
        BOOST_PP_SEQ_FOR_EACH( FC_API_VTABLE_DEFINE_VISIT, CLASS, METHODS ) \
      } \
  }; \
}  

