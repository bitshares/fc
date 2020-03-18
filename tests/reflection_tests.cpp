#include <boost/test/unit_test.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>

#include <type_traits>

struct reflect_test_base {
   int x = 1;
   char y = 'a';
};
struct reflect_test_derived : reflect_test_base {
   double z = 3.14;
};
struct reflect_layer_1 { reflect_test_base b; int32_t n; };
struct reflect_layer_2 { reflect_layer_1 l1; reflect_test_derived d; };
struct reflect_layer_3 { reflect_layer_2 l2; int32_t i; };

struct reflect_test_internal {
   int x = 7;
   char y = 'A';

   FC_REFLECT_INTERNAL(reflect_test_internal, (x)(y))
};

template<typename T, typename = void>
struct reflect_internal_sfinae {
   int x = 100;
   FC_REFLECT_INTERNAL(reflect_internal_sfinae, (x))
};
template<typename T>
struct reflect_internal_sfinae<T, std::enable_if_t<std::is_same<T, char>{}>> {
   int x = 200;
   std::string y = "hi";
   FC_REFLECT_INTERNAL(reflect_internal_sfinae, (x)(y))
};

FC_REFLECT( reflect_test_base, (x)(y) );
FC_REFLECT_DERIVED( reflect_test_derived, (reflect_test_base), (z) );
FC_REFLECT( reflect_layer_1, (b)(n) );
FC_REFLECT( reflect_layer_2, (l1)(d) );
FC_REFLECT( reflect_layer_3, (l2)(i) );
FC_COMPLETE_INTERNAL_REFLECTION(reflect_test_internal);
FC_COMPLETE_INTERNAL_REFLECTION_TEMPLATE((typename T), reflect_internal_sfinae<T>);

BOOST_AUTO_TEST_SUITE( fc_reflection )

BOOST_AUTO_TEST_CASE( reflection_static_tests )
{
   // These are all compile-time tests, nothing actually happens here at runtime
   using namespace fc::typelist;
   using base_reflection = fc::reflector<reflect_test_base>;
   using derived_reflection = fc::reflector<reflect_test_derived>;
   static_assert(length<base_reflection::members>() == 2, "");
   static_assert(length<derived_reflection::members>() == 3, "");
   static_assert(at<derived_reflection::members, 0>::is_derived, "");
   static_assert(std::is_same<at<derived_reflection::members, 0>::field_container, reflect_test_base>::value, "");
   static_assert(at<derived_reflection::members, 1>::is_derived, "");
   static_assert(std::is_same<at<derived_reflection::members, 1>::field_container, reflect_test_base>::value, "");
   static_assert(at<derived_reflection::members, 2>::is_derived == false, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 0, 1>, list<int>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 0, 2>, list<int, bool>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 0, 3>, list<int, bool, char>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 1, 3>, list<bool, char>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 2, 3>, list<char>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 1, 2>, list<bool>>::value, "");
   static_assert(std::is_same<slice<list<int, bool, char>, 1>, list<bool, char>>::value, "");
   static_assert(std::is_same<make_sequence<0>, list<>>::value, "");
   static_assert(std::is_same<make_sequence<1>, list<std::integral_constant<size_t, 0>>>::value, "");
   static_assert(std::is_same<make_sequence<2>,
                              list<std::integral_constant<size_t, 0>, std::integral_constant<size_t, 1>>>::value, "");
   static_assert(std::is_same<make_sequence<3>, list<std::integral_constant<size_t, 0>,
                                                     std::integral_constant<size_t, 1>,
                                                     std::integral_constant<size_t, 2>>>::value, "");
   static_assert(std::is_same<zip<list<>, list<>>, list<>>::value, "");
   static_assert(std::is_same<zip<list<bool>, list<char>>, list<list<bool, char>>>::value, "");
   static_assert(std::is_same<zip<list<int, bool>,
                              list<char, double>>, list<list<int, char>, list<bool, double>>>::value, "");
   static_assert(std::is_same<fc::typelist::index<list<>>, list<>>::value, "");
   static_assert(std::is_same<fc::typelist::index<list<int, bool, char, double>>,
                              list<list<std::integral_constant<size_t, 0>, int>,
                                   list<std::integral_constant<size_t, 1>, bool>,
                                   list<std::integral_constant<size_t, 2>, char>,
                                   list<std::integral_constant<size_t, 3>, double>>>::value, "");
   static_assert(length<fc::reflector<reflect_test_internal>::members>() == 2, "");
   static_assert(std::is_same<at<fc::reflector<reflect_test_internal>::members, 0>::type, int>{}, "");
   static_assert(std::is_same<at<fc::reflector<reflect_test_internal>::members, 1>::type, char>{}, "");
   static_assert(length<fc::reflector<reflect_internal_sfinae<bool>>::members>() == 1, "");
   static_assert(length<fc::reflector<reflect_internal_sfinae<char>>::members>() == 2, "");
   static_assert(std::is_same<prepend<list<>, bool>, list<bool>>{}, "");
   static_assert(std::is_same<prepend<list<int>, bool>, list<bool, int>>{}, "");
   static_assert(std::is_same<prepend<list<int, char>, bool>, list<bool, int, char>>{}, "");
   static_assert(std::is_same<append<list<>, bool>, list<bool>>{}, "");
   static_assert(std::is_same<append<list<int>, bool>, list<int, bool>>{}, "");
   static_assert(std::is_same<append<list<int, char>, bool>, list<int, char, bool>>{}, "");
   static_assert(std::is_same<insert<list<>, bool, 0>, list<bool>>{}, "");
   static_assert(std::is_same<insert<list<char>, bool, 0>, list<bool, char>>{}, "");
   static_assert(std::is_same<insert<list<char>, bool, 1>, list<char, bool>>{}, "");
   static_assert(std::is_same<insert<list<int, char>, bool, 0>, list<bool, int, char>>{}, "");
   static_assert(std::is_same<insert<list<int, char>, bool, 1>, list<int, bool, char>>{}, "");
   static_assert(std::is_same<insert<list<int, char>, bool, 2>, list<int, char, bool>>{}, "");
}

BOOST_AUTO_TEST_CASE( object_reflection_tests )
{
   reflect_test_derived obj;
   fc::object_reflection<reflect_test_derived> reflection(obj);
   BOOST_CHECK_EQUAL(obj.x, reflection.x());
   BOOST_CHECK_EQUAL(obj.y, reflection.y());
   BOOST_CHECK_EQUAL(obj.z, reflection.z());
   reflection.x() = 77;
   reflection.z() = 1;
   BOOST_CHECK_EQUAL(obj.x, 77);
   BOOST_CHECK_EQUAL(obj.z, 1);
   fc::object_reflection<const reflect_test_derived> const_reflection(obj);
   BOOST_CHECK_EQUAL(obj.x, const_reflection.x());
   BOOST_CHECK_EQUAL(obj.y, const_reflection.y());
   BOOST_CHECK_EQUAL(obj.z, const_reflection.z());
}

BOOST_AUTO_TEST_CASE( typelist_dispatch_test )
{
   using list = fc::typelist::list<float, bool, char>;
   auto get_name = [](auto t) -> std::string { return fc::get_typename<typename decltype(t)::type>::name(); };
   BOOST_CHECK_EQUAL(fc::typelist::runtime::dispatch(list(), 0ul, get_name), "float");
   BOOST_CHECK_EQUAL(fc::typelist::runtime::dispatch(list(), 1ul, get_name), "bool");
   BOOST_CHECK_EQUAL(fc::typelist::runtime::dispatch(list(), 2ul, get_name), "char");
}

struct internal_checker {
   reflect_test_internal& rti;
   template<typename Value, typename Container, Value Container::*ptr,
            std::enable_if_t<std::is_same<Value, int>{}, bool> = true>
   void operator()(const char* name) const {
      BOOST_CHECK(std::string(name) == "x");
      BOOST_CHECK(rti.*ptr == 7);
   }
   template<typename Value, typename Container, Value Container::*ptr,
            std::enable_if_t<std::is_same<Value, char>{}, bool> = true>
   void operator()(const char* name) const {
      BOOST_CHECK(std::string(name) == "y");
      BOOST_CHECK(rti.*ptr == 'A');
   }
};

// Helper template to use fc::typelist::at without a comma, for macro friendliness
template<typename T> struct index_from { template<std::size_t idx> using at = fc::typelist::at<T, idx>; };
BOOST_AUTO_TEST_CASE( reflection_get_test )
{ try {
   reflect_test_derived derived;
   reflect_test_base& base = derived;

   using base_reflector = fc::reflector<reflect_test_base>;
   using derived_reflector = fc::reflector<reflect_test_derived>;

   BOOST_CHECK(index_from<base_reflector::members>::at<0>::get(base) == 1);
   BOOST_CHECK(index_from<base_reflector::members>::at<1>::get(base) == 'a');

   fc::typelist::at<base_reflector::members, 0>::get(base) = 5;
   fc::typelist::at<base_reflector::members, 1>::get(base) = 'q';

   BOOST_CHECK(index_from<base_reflector::members>::at<0>::get(base) == 5);
   BOOST_CHECK(index_from<base_reflector::members>::at<1>::get(base) == 'q');

   BOOST_CHECK(index_from<derived_reflector::members>::at<0>::get(derived) == 5);
   BOOST_CHECK(index_from<derived_reflector::members>::at<1>::get(derived) == 'q');
   BOOST_CHECK(index_from<derived_reflector::members>::at<2>::get(derived) == 3.14);

   fc::typelist::at<derived_reflector::members, 1>::get(derived) = 'X';

   BOOST_CHECK(index_from<base_reflector::members>::at<1>::get(base) == 'X');

   reflect_layer_3 l3;
   BOOST_CHECK(index_from<index_from<index_from<index_from<fc::reflector<reflect_layer_3>::members>::at<0>
               ::reflector::members>::at<0>::reflector::members>::at<0>::reflector::members>::at<1>::get(l3.l2.l1.b)
               == 'a');
   BOOST_CHECK(index_from<index_from<index_from<fc::reflector<reflect_layer_3>::members>::at<0>::reflector::members>
               ::at<1>::reflector::members>::at<1>::get(l3.l2.d) == 'a');
   BOOST_CHECK(index_from<index_from<index_from<fc::reflector<reflect_layer_3>::members>::at<0>::reflector::members>
               ::at<1>::reflector::members>::at<2>::get(l3.l2.d) == 3.14);

   reflect_test_internal internal;
   internal_checker checker{internal};
   fc::reflector<reflect_test_internal>::visit(checker);

   BOOST_CHECK_EQUAL(fc::json::to_string(reflect_internal_sfinae<int>()), std::string("{\"x\":100}"));
   BOOST_CHECK_EQUAL(fc::json::to_string(reflect_internal_sfinae<char>()), std::string("{\"x\":200,\"y\":\"hi\"}"));
} FC_CAPTURE_LOG_AND_RETHROW( (0) ) }

BOOST_AUTO_TEST_SUITE_END()
