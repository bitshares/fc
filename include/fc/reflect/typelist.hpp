#pragma once
/**
 * @file fc/reflect/typelist.hpp
 *
 * @brief Defines a template for manipulating and storing compile-time lists of types
 */

#include <type_traits>
#include <functional>

namespace fc {

/// This namespace contains the list type, and all of the operations and queries which can be performed upon it
namespace typelist {

// Forward declare the list so impl can see it
template<typename...> struct list;

namespace impl {
using typelist::list;

template<typename, template<typename...> class> struct apply;
template<typename... Ts, template<typename...> class Delegate>
struct apply<list<Ts...>, Delegate> { using type = Delegate<Ts...>; };

template<typename... Ts>
struct length;
template<> struct length<> { constexpr static std::size_t value = 0; };
template<typename T, typename... Ts>
struct length<T, Ts...> { constexpr static std::size_t value = length<Ts...>::value+1; };

template<typename...> struct concat;
template<typename... OldTypes, typename... NewTypes>
struct concat<list<OldTypes...>, list<NewTypes...>> {
   using type = list<OldTypes..., NewTypes...>;
};
template<typename... OldTypes, typename... NewTypes, typename NextList, typename... Lists>
struct concat<list<OldTypes...>, list<NewTypes...>, NextList, Lists...> {
   using type = typename concat<list<OldTypes..., NewTypes...>, NextList, Lists...>::type;
};

template<std::size_t count> struct make_sequence;
template<> struct make_sequence<0> { using type = list<>; };
template<> struct make_sequence<1> { using type = list<std::integral_constant<std::size_t, 0>>; };
template<std::size_t count>
struct make_sequence {
   using type = typename concat<typename make_sequence<count-1>::type,
                                list<std::integral_constant<std::size_t, count-1>>>::type;
};

template<typename, typename> struct transform;
template<typename... List, typename Transformer>
struct transform<list<List...>, Transformer> {
   using type = list<typename Transformer::template transform<List>::type...>;
};

template<typename Search, typename List> struct index_of;
template<typename Search> struct index_of<Search, list<>> { constexpr static int value = -1; };
template<typename Search, typename T, typename... Ts>
struct index_of<Search, list<T, Ts...>> {
    constexpr static int deeper = index_of<Search, list<Ts...>>::value;
    constexpr static int value = std::is_same<Search, T>::value? 0 : (deeper == -1? -1 : deeper + 1);
};

template<typename...> struct concat_unique;
template<typename... Uniques>
struct concat_unique<list<Uniques...>, list<>> {
   using type = list<Uniques...>;
};
template<typename... Uniques, typename T>
struct concat_unique<list<Uniques...>, list<T>> {
   using type = std::conditional_t<index_of<T, list<Uniques...>>::value >= 0,
                                   list<Uniques...>, list<Uniques..., T>>;
};
template<typename... Uniques, typename T1, typename T2, typename... Types>
struct concat_unique<list<Uniques...>, list<T1, T2, Types...>> {
   using type = typename concat_unique<
      typename concat_unique<list<Uniques...>, list<T1>>::type, list<T2, Types...>>::type;
};
template<typename... Uniques, typename... Lists>
struct concat_unique<list<Uniques...>, list<>, Lists...> {
   using type = typename concat_unique<list<Uniques...>, Lists...>::type;
};
template<typename Uniques, typename L1a, typename... L1s, typename L2, typename... Lists>
struct concat_unique<Uniques, list<L1a, L1s...>, L2, Lists...> {
   using type = typename concat_unique<typename concat_unique<Uniques, list<L1a, L1s...>>::type, L2, Lists...>::type;
};

template<typename, std::size_t> struct at;
template<typename T, typename... Types>
struct at<list<T, Types...>, 0> { using type = T; };
template<typename T, typename... Types, std::size_t index>
struct at<list<T, Types...>, index> : at<list<Types...>, index-1> {};

template<typename, typename, std::size_t> struct remove_at;
template<typename... Left, typename T, typename... Right>
struct remove_at<list<Left...>, list<T, Right...>, 0> { using type = list<Left..., Right...>; };
template<typename... Left, typename T, typename... Right, std::size_t index>
struct remove_at<list<Left...>, list<T, Right...>, index> {
   using type = typename remove_at<list<Left..., T>, list<Right...>, index-1>::type;
};

template<template<typename> class Filter, typename Filtered, typename List> struct filter;
template<template<typename> class Filter, typename... Filtered>
struct filter<Filter, list<Filtered...>, list<>> { using type = list<Filtered...>; };
template<template<typename> class Filter, typename... Filtered, typename T1, typename... Types>
struct filter<Filter, list<Filtered...>, list<T1, Types...>> {
   using type = typename std::conditional_t<Filter<T1>::value,
                                           filter<Filter, list<Filtered..., T1>, list<Types...>>,
                                           filter<Filter, list<Filtered...>, list<Types...>>>::type;
};

template<typename, typename, std::size_t, std::size_t, typename = void> struct slice;
template<typename... Results, typename... Types, std::size_t index>
struct slice<list<Results...>, list<Types...>, index, index, void> { using type = list<Results...>; };
template<typename... Results, typename T, typename... Types, std::size_t end>
struct slice<list<Results...>, list<T, Types...>, 0, end, std::enable_if_t<end != 0>>
        : slice<list<Results..., T>, list<Types...>, 0, end-1> {};
template<typename T, typename... Types, std::size_t start, std::size_t end>
struct slice<list<>, list<T, Types...>, start, end, std::enable_if_t<start != 0>>
        : slice<list<>, list<Types...>, start-1, end-1> {};

template<typename, typename> struct zip;
template<>
struct zip<list<>, list<>> { using type = list<>; };
template<typename A, typename... As, typename B, typename... Bs>
struct zip<list<A, As...>, list<B, Bs...>> {
   using type = typename concat<list<list<A, B>>, typename zip<list<As...>, list<Bs...>>::type>::type;
};

template<typename, template<typename> class> struct any;
template<template<typename> class Predicate>
struct any<list<>, Predicate> {
   constexpr static bool value = false;
};
template<typename T, template<typename> class Predicate>
struct any<list<T>, Predicate> {
   constexpr static bool value = Predicate<T>::value;
};
template<typename T1, typename T2, typename... Ts, template<typename> class Predicate>
struct any<list<T1, T2, Ts...>, Predicate> {
   constexpr static bool value = Predicate<T1>::value || any<list<T2, Ts...>, Predicate>::value;
};

template<typename Callable, typename Ret, typename T>
Ret dispatch_helper(Callable& c) { return c(T()); }

} // namespace impl

/// The actual list type
template<typename... Types>
struct list { using type = list; };

/// Apply a list of types as arguments to another template
template<typename List, template<typename...> class Delegate>
using apply = typename impl::apply<List, Delegate>::type;

/// Get the number of types in a list
template<typename List>
constexpr static std::size_t length() { return apply<List, impl::length>::value; }

/// Concatenate two or more typelists together
template<typename... Lists>
using concat = typename impl::concat<Lists...>::type;

/// Create a list of sequential integers ranging from [0, count)
template<std::size_t count>
using make_sequence = typename impl::make_sequence<count>::type;

/// Template to build typelists using the following syntax:
/// @code builder<>::type::add<T1>::add<T2>::add<T3>[...]::finalize @endcode
/// Or:
/// @code builder<>::type::add_list<list<T1, T2>>::add_list<T3, T4>>[...]::finalize @endcode
template<typename List = list<>>
struct builder {
   template<typename NewType> using add = typename builder<typename impl::concat<List, list<NewType>>::type>::type;
   template<typename NewList> using add_list = typename builder<typename impl::concat<List, NewList>::type>::type;
   using type = builder;
   using finalize = List;
};

/// Transform elements of a typelist
template<typename List, typename Transformer>
using transform = typename impl::transform<List, Transformer>::type;

/// Get the index of the given type within a list, or -1 if type is not found
template<typename List, typename T>
constexpr static int index_of() { return impl::index_of<T, List>::value; }

/// Check if a given type is in a list
template<typename List, typename T>
constexpr static bool contains() { return impl::index_of<T, List>::value != -1; }

/// Remove duplicate items from one or more typelists and concatenate them all together
template<typename... TypeLists>
using concat_unique = typename impl::concat_unique<list<>, TypeLists...>::type;

/// Get the type at the specified list index
template<typename List, std::size_t index>
using at = typename impl::at<List, index>::type;

/// Get the type at the beginning of the list
template<typename List>
using first = at<List, 0>;
/// Get the type at the end of the list
template<typename List>
using last = at<List, length<List>()-1>;

/// Get the list with the element at the given index removed
template<typename List, std::size_t index>
using remove_at = typename impl::remove_at<list<>, List, index>::type;

/// Get the list with the given type removed
template<typename List, typename Remove>
using remove_element = remove_at<List, index_of<List, Remove>()>;

/// Get a list with all elements that do not pass a filter removed
template<typename List, template<typename> class Filter>
using filter = typename impl::filter<Filter, list<>, List>::type;

/// Template to invert a filter, i.e. filter<mylist, filter_inverter<myfilter>::type>
template<template<typename> class Filter>
struct invert_filter {
   template<typename T>
   struct type { constexpr static bool value = !Filter<T>::value; };
};

/// Take the sublist at indexes [start, end)
template<typename List, std::size_t start, std::size_t end = length<List>()>
using slice = typename impl::slice<list<>, List, start, end>::type;

/// Zip two equal-length typelists together, i.e. zip<list<X, Y>, list<A, B>> == list<list<X, A>, list<Y, B>>
template<typename ListA, typename ListB>
using zip = typename impl::zip<ListA, ListB>::type;

/// Add indexes to types in the list, i.e. index<list<A, B, C>> == list<list<0, A>, list<1, B>, list<2, C>> where
/// 0, 1, and 2 are std::integral_constants of type std::size_t
template<typename List>
using index = typename impl::zip<typename impl::make_sequence<length<List>()>::type, List>::type;

/// Determine whether any type in list passes predicate
template<typename List, template<typename> class Predicate>
constexpr static bool any = impl::any<List, Predicate>::value;

/// A wrapper which wraps an uninstantiated template in a type so it can be handled in a typelist
template<template<typename...> class Template>
struct wrapped {
   template<typename... Ts>
   using unwrap = Template<Ts...>;
};

/// This namespace contains some utilities that provide runtime operations on typelists
namespace runtime {
/// Type wrapper object allowing arbitrary types to be passed to functions as information rather than data
template<typename T> struct wrapper { using type = T; };

/**
 * @brief Index into the typelist for a type T, and invoke the callable with an argument wrapper<T>()
 * @param index Index of the type in the typelist to invoke the callable with
 * @param c The callable to invoke
 * @return The value returned by the callable
 * @note The callable return type must be the same for all list elements
 *
 * If index is out of bounds, throws std::out_of_range exception
 */
template<typename... Types, typename Callable, typename = std::enable_if_t<impl::length<Types...>::value != 0>,
         typename Return = decltype(std::declval<Callable>()(wrapper<at<list<Types...>, 0>>()))>
Return dispatch(list<Types...>, std::size_t index, Callable c) {
   static std::function<Return(Callable&)> call_table[] =
      { impl::dispatch_helper<Callable, Return, wrapper<Types>>... };
   if (index < impl::length<Types...>::value) return call_table[index](c);
   throw std::out_of_range("Invalid index to fc::typelist::runtime::dispatch()");
}
template<typename List, typename Callable>
auto dispatch(List l, int64_t index, Callable c) {
   if (index < 0) throw std::out_of_range("Negative index to fc::typelist::runtime::dispatch()");
   return dispatch(l, std::size_t(index), std::move(c));
}

/// @brief Invoke the provided callable with an argument wrapper<Type>() for each type in the list
template<typename... Types, typename Callable>
void for_each(list<Types...>, Callable c) {
   bool trues[] = { [](Callable& c, auto t) { c(t); return true; }(c, wrapper<Types>())... };
   (void)(trues);
}

} } } // namespace fc::typelist::runtime
