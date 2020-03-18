#pragma once
/**
 * @file fc/reflect/reflect.hpp
 *
 * @brief Defines types and macros used to provide reflection.
 *
 */

#include <fc/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <stdint.h>
#include <string.h>
#include <type_traits>

#include <fc/reflect/typename.hpp>
#include <fc/reflect/typelist.hpp>

namespace fc {

template<typename> struct reflector;
namespace member_names {
/// A template which stores the name of the native member at a given index in a given class
template<typename Class, std::size_t index> struct member_name {
   constexpr static const char* value = "Unknown member";
};
}

/**
 *  @brief A template to store compile-time information about a field in a reflected struct
 *
 *  @tparam Container The type of the struct or class containing the field
 *  @tparam Member The type of the field
 *  @tparam field A pointer-to-member for the reflected field
 */
template<std::size_t Index, typename Container, typename Member, Member Container::*field>
struct field_reflector {
   using container = Container;
   using type = Member;
   using reflector = fc::reflector<type>;
   constexpr static std::size_t index = Index;
   constexpr static bool is_derived = false;
   constexpr static type container::*pointer = field;

   /// @brief Given a reference to the container type, get a reference to the field
   static type& get(container& c) { return c.*field; }
   static const type& get(const container& c) { return c.*field; }
   /// @brief Get the name of the field
   static const char* get_name() { return fc::member_names::member_name<container, index>::value; }
};
/// Basically the same as @ref field_reflection, but for inherited fields
/// Note that inherited field reflections do not have an index field; indexes are for native fields only
template<std::size_t IndexInBase, typename Base, typename Derived, typename Member, Member Base::*field>
struct inherited_field_reflector {
   using container = Derived;
   using field_container = Base;
   using type = Member;
   using reflector = fc::reflector<type>;
   constexpr static std::size_t index_in_base = IndexInBase;
   constexpr static bool is_derived = true;
   constexpr static type field_container::*pointer = field;

   static type& get(container& c) {
      // And we need a distinct inherited_field_reflection type because this conversion can't be done statically
      type container::* derived_field = field;
      return c.*derived_field;
   }
   static const type& get(const container& c) {
      type container::* derived_field = field;
      return c.*derived_field;
   }
   static const char* get_name() {
      using Reflector = typename fc::reflector<Base>::native_members::template at<IndexInBase>;
      return Reflector::get_name();
   }
};

template<typename, typename, bool> struct basic_field_reflection;
/**
 * @brief An object reflection is a mirror-image of an object for which a reflector is defined.
 * @tparam Object The type of the object this reflection mirrors
 * @tparam Data A pointer to this type will be passed to the constructor of each FieldReflection to initialize it
 * @tparam FieldReflection A template to instantiate for each field to make the reflections of the fields. The
 * constructor will be passed a reference to the reflected field and a pointer to the Data object.
 * @tparam Reflectors Used when the object_reflection is a field of another object_reflection, this holds the path of
 * reflectors leading to the field this object_reflection reflects
 *
 * The object reflection has fields with names matching those of the reflected object. These fields are callable
 * objects which return a type which can be treated like a reference to the field value. If the field's type is
 * complex, an object_reflection of that field's object is returned.
 *
 * By default, the object_reflection is very simple and provides no additional functionality. The following two lines
 * would be equivalent:
 *
 * \code{.cpp}
 * my_object.int_field = 2;
 * my_object_reflection.int_field() = 2;
 * \endcode
 *
 * The real power lies in the fact that the field reflections are based on a template, meaning that the reflection
 * can be instrumented by client code to do additional checking. For instance, a reflection template could be used
 * which records which fields in the reflection were accessed, or which fields were written and what values that were
 * read/written when.
 *
 * The default template is not instantiable, but provides example code for a reflection of the following type:
 * \code{.cpp}
 * struct Object {
 *    int int_field;
 *    const char char_field;
 * };
 * \endcode
 *
 * The object_reflection constructor takes a pointer to a Data object which will be distributed to all field
 * reflections and subobject reflections. Memory for this pointer is managed by the client code, and the pointer is
 * expected to remain valid for the lifetime of the object_reflection and all its children. The pointer is not used
 * by the default basic_field_reflection types and may be null.
 */
template<typename Object, typename Data = void,
         template<typename, typename, bool> class FieldReflection = basic_field_reflection,
         typename Reflectors = typelist::list<>>
struct object_reflection { constexpr static bool is_defined = false; Object& _ref_; Data* _data_; };

/// Implementation of @ref basic_field_reflection -- has SFINAE template parameters
// Default template: mutable unreflected types
template<typename Reflectors, typename Data, bool is_const = false, typename = void>
struct basic_field_reflection_impl {
   using Field = typename typelist::last<Reflectors>::type;
   Field& ref;
   Data* data;

   const Field& operator()() const { return ref; }
   Field& operator()() { return ref; }
   operator Field&() { return ref; }
   operator const Field&() const { return ref; }
};
// Specialization for const unreflected types
template<typename Reflectors, typename Data>
struct basic_field_reflection_impl<Reflectors, Data, true, void> {
   using Field = typename typelist::last<Reflectors>::type;
   const Field& ref;
   Data* data;

   const Field& operator()() const { return ref; }
   operator const Field&() const { return ref; }
};
// Specialization for mutable reflected types
template<typename Reflectors, typename Data>
struct basic_field_reflection_impl<Reflectors, Data, false,
      std::enable_if_t<fc::object_reflection<typename typelist::last<Reflectors>::type>::is_defined>>
{
   using Field = typename typelist::last<Reflectors>::type;
   Field& ref;
   Data* data;

   const object_reflection<Field> operator()() const { return object_reflection<Field, Reflectors>(ref); }
   object_reflection<Field> operator()() { return object_reflection<Field, Reflectors>(ref); }
   operator const Field&() const { return ref; }
   operator Field&() { return ref; }
};
// Specialization for const reflected types
template<typename Reflectors, typename Data>
struct basic_field_reflection_impl<Reflectors, Data, true,
      std::enable_if_t<fc::object_reflection<typename typelist::last<Reflectors>::type>::is_defined>>
{
   using Field = typename typelist::last<Reflectors>::type;
   const Field& ref;
   Data* data;

   const object_reflection<Field> operator()() const { return object_reflection<const Field, Reflectors>(ref); }
   operator const Field&() const { return ref; }
};
/**
 * @brief A template for a basic field reflection that simply returns a reference to the reflected field
 * @tparam Reflectors Stack of reflectors of the fields in the path to the reflected field
 * @tparam Data Type of object to initialize the field reflection with
 * @tparam is_const A boolean specifying whether the field is on a const object or not
 *
 * A field reflection is a mirror image of a field. Field reflections appear in object reflections, which are defined
 * below. Field reflections are callable objects which have the same name as the field they reflect, and usually are
 * intended to be treated as though they return a reference to that field.
 *
 * This class provides a basic field reflection type which provides no additional functionality, but simply allows
 * reference access to the field
 */
template<typename Reflectors, typename Data = void, bool is_const = false>
struct basic_field_reflection : basic_field_reflection_impl<Reflectors, Data, is_const> {
   basic_field_reflection(typename basic_field_reflection::Field& ref, Data* data)
      : basic_field_reflection_impl<Reflectors, Data, is_const>{ref, data} {}
};
template<typename Reflectors, typename Data>
struct basic_field_reflection<Reflectors, Data, true> : basic_field_reflection_impl<Reflectors, Data, true> {
   basic_field_reflection(const typename basic_field_reflection::Field& ref, Data* data)
      : basic_field_reflection_impl<Reflectors, Data, true>{ref, data} {}
};

/// Macro to define a member reflection; intended to be used within BOOST_PP_SEQ_FOR_EACH_I
#define FC_MEMBER_REFLECTION(R, X, IDX, MEMBER) IndexReflection<IDX> MEMBER;
#define FC_INIT_MEMBER_REFLECTION(R, X, MEMBER) , MEMBER(_ref_.MEMBER, _data_)
#define FC_OBJECT_REFLECTION_BASES(R, CONST, IDX, BASE) \
   BOOST_PP_IF(IDX, BOOST_PP_COMMA, : BOOST_PP_EMPTY)() object_reflection<CONST BASE, Data, FR, Reflectors>
#define FC_INIT_OBJECT_REFLECTION_BASE(R, CONST, BASE) object_reflection<CONST BASE, Data, FR, Reflectors>(ref, data),
#define FC_OBJECT_REFLECTION_IMPL(TYPE, MEMBERS, BASES, CONST) \
      constexpr static bool is_defined = true; \
      TYPE& _ref_; \
      Data* _data_; \
      template<size_t index> \
      using IndexReflection = \
         FR<typelist::append<Reflectors, \
                             typelist::at<typename reflector<std::decay_t<TYPE>>::native_members, index>>, \
            Data, std::is_const<TYPE>::value>; \
      BOOST_PP_SEQ_FOR_EACH_I(FC_MEMBER_REFLECTION, X, MEMBERS) \
      object_reflection(TYPE& ref, Data* data = nullptr) \
       : BOOST_PP_SEQ_FOR_EACH(FC_INIT_OBJECT_REFLECTION_BASE, CONST, BASES) _ref_(ref), _data_(data) \
         BOOST_PP_SEQ_FOR_EACH(FC_INIT_MEMBER_REFLECTION, X, MEMBERS) {} \
      operator const std::decay_t<TYPE>&() const { return _ref_; }
/// Macro to define an @ref object_reflection for a supplied type
#define FC_OBJECT_REFLECTION(TYPE, MEMBERS) \
   namespace fc { \
   template<typename Data, template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<TYPE, Data, FR, Reflectors> { \
      FC_OBJECT_REFLECTION_IMPL(TYPE, MEMBERS, , ) \
   }; \
   template<typename Data, template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<const TYPE, Data, FR, Reflectors> { \
      FC_OBJECT_REFLECTION_IMPL(const TYPE, MEMBERS, , const) \
   }; }
#define FC_OBJECT_REFLECTION_DERIVED(TYPE, INHERITS, MEMBERS) \
   template<typename Data, template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<TYPE, Data, FR, Reflectors> \
         BOOST_PP_SEQ_FOR_EACH_I(FC_OBJECT_REFLECTION_BASES, , INHERITS) { \
      FC_OBJECT_REFLECTION_IMPL(TYPE, MEMBERS, INHERITS, ) \
   }; \
   template<typename Data, template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<const TYPE, Data, FR, Reflectors> \
         BOOST_PP_SEQ_FOR_EACH_I(FC_OBJECT_REFLECTION_BASES, const, INHERITS) { \
      FC_OBJECT_REFLECTION_IMPL(const TYPE, MEMBERS, INHERITS, const) \
   };
#define FC_OBJECT_REFLECTION_DERIVED_TEMPLATE(TEMPLATES, TYPE, INHERITS, MEMBERS) \
   template<BOOST_PP_SEQ_ENUM(TEMPLATES), typename Data, \
            template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<TYPE, Data, FR, Reflectors> \
         BOOST_PP_SEQ_FOR_EACH_I(FC_OBJECT_REFLECTION_BASES, , INHERITS) { \
      FC_OBJECT_REFLECTION_IMPL(TYPE, MEMBERS, INHERITS, ) \
   }; \
   template<BOOST_PP_SEQ_ENUM(TEMPLATES), typename Data, \
            template<typename, typename, bool> class FR, typename Reflectors> \
   struct object_reflection<const TYPE, Data, FR, Reflectors> \
         BOOST_PP_SEQ_FOR_EACH_I(FC_OBJECT_REFLECTION_BASES, const, INHERITS) { \
      FC_OBJECT_REFLECTION_IMPL(const TYPE, MEMBERS, INHERITS, const) \
   };

namespace impl {
/// Template to make a transformer of a @ref field_reflection from a base class to a derived class
template<typename Derived>
struct Derivation_reflector_transformer {
   template<typename> struct transform;
   template<std::size_t IndexInBase, typename BaseContainer, typename Member, Member BaseContainer::*field>
   struct transform<field_reflector<IndexInBase, BaseContainer, Member, field>> {
       using type = inherited_field_reflector<IndexInBase, BaseContainer, Derived, Member, field>;
   };
   template<std::size_t IndexInBase, typename BaseContainer, typename IntermediateContainer,
            typename Member, Member BaseContainer::*field>
   struct transform<inherited_field_reflector<IndexInBase, BaseContainer, IntermediateContainer, Member, field>> {
       using type = inherited_field_reflector<IndexInBase, BaseContainer, Derived, Member, field>;
   };
};
} // namespace impl

/// Macro to transform reflected fields of a base class to a derived class and concatenate them to a type list
#define FC_CONCAT_BASE_MEMBER_REFLECTIONS(r, derived, base) \
   ::add_list<typelist::transform<reflector<base>::members, \
                                  impl::Derivation_reflector_transformer<derived>::template transform>>
/// Macro to concatenate a new @ref field_reflection to a typelist
#define FC_CONCAT_MEMBER_REFLECTION(r, container, idx, member) \
   ::add<field_reflector<idx, container, decltype(container::member), &container::member>>
#define FC_REFLECT_MEMBER_NAME(r, container, idx, member) \
   template<> struct member_name<container, idx> { constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
#define FC_REFLECT_TEMPLATE_MEMBER_NAME(r, data, idx, member) \
   template<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(0, data))> struct member_name<BOOST_PP_SEQ_ELEM(1, data), idx> { \
    constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
/// Macro to concatenate a new type to a typelist
#define FC_CONCAT_TYPE(r, x, TYPE) ::add<TYPE>

/**
 *  @brief defines visit functions for T
 *  Unless this is specialized, visit() will not be defined for T.
 *
 *  @tparam T - the type that will be visited.
 *
 *  The @ref FC_REFLECT(TYPE,MEMBERS) or FC_STATIC_REFLECT_DERIVED(TYPE,BASES,MEMBERS) macro is used to specialize this
 *  class for your type.
 */
template<typename T>
struct reflector{
    typedef T type;
    typedef std::false_type is_defined;
    /// A typelist with a @ref field_reflector for each native member (non-inherited) of the struct
    using native_members = typelist::list<>;
    /// A typelist with a @ref inherited_field_reflector for each inherited member of the struct
    using inherited_members = typelist::list<>;
    /// A typelist with a {inherited_,}field_reflector for each member of the struct, starting with inherited members
    using members = typelist::list<>;
    /// A typelist of base classes for this type
    using base_classes = typelist::list<>;

    /**
     *  @tparam Visitor a function object of the form:
     *
     *    @code
     *     struct functor {
     *        template<typename Member, class Class, Member (Class::*member)>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  If T is an enum then the functor has the following form:
     *    @code
     *     struct functor {
     *        template<int Value>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  @param v a functor that will be called for each member on T
     *
     *  @note - this method is not defined for non-reflected types.
     */
    #ifdef DOXYGEN
    template<typename Visitor>
    static inline void visit( const Visitor& v );
    #endif // DOXYGEN
};

void throw_bad_enum_cast( int64_t i, const char* e );
void throw_bad_enum_cast( const char* k, const char* e );
} // namespace fc


#ifndef DOXYGEN

#define FC_REFLECT_VISIT_BASE(r, visitor, base) \
  fc::reflector<base>::visit( visitor );


#ifndef _MSC_VER
  #define TEMPLATE template
#else
  // Disable warning C4482: nonstandard extention used: enum 'enum_type::enum_value' used in qualified name
  #pragma warning( disable: 4482 )
  #define TEMPLATE
#endif

#define FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
{ typedef decltype(((type*)nullptr)->elem) member_type;  \
  visitor.TEMPLATE operator()<member_type,type,&type::elem>( BOOST_PP_STRINGIZE(elem) ); \
}

#define FC_REFLECT_VISIT_MEMBER_I( r, visitor, I, elem ) \
   case I: FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) break;


#define FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}

#endif // DOXYGEN


#define FC_REFLECT_VISIT_ENUM( r, enum_type, elem ) \
  v.operator()(BOOST_PP_STRINGIZE(elem), int64_t(enum_type::elem) );
#define FC_REFLECT_ENUM_TO_STRING( r, enum_type, elem ) \
   case enum_type::elem: return BOOST_PP_STRINGIZE(elem);
#define FC_REFLECT_ENUM_TO_FC_STRING( r, enum_type, elem ) \
   case enum_type::elem: return std::string(BOOST_PP_STRINGIZE(elem));

#define FC_REFLECT_ENUM_FROM_STRING( r, enum_type, elem ) \
  if( strcmp( s, BOOST_PP_STRINGIZE(elem)  ) == 0 ) return enum_type::elem;
#define FC_REFLECT_ENUM_FROM_STRING_CASE( r, enum_type, elem ) \
   case enum_type::elem:

#define FC_REFLECT_ENUM( ENUM, FIELDS ) \
namespace fc { \
template<> struct reflector<ENUM> { \
    typedef std::true_type is_defined; \
    static const char* to_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_STRING, ENUM, FIELDS ) \
        default: \
           fc::throw_bad_enum_cast( fc::to_string(int64_t(elem)).c_str(), BOOST_PP_STRINGIZE(ENUM) ); \
      }\
      return nullptr; \
    } \
    static const char* to_string(int64_t i) { \
      return to_string(ENUM(i)); \
    } \
    static std::string to_fc_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_FC_STRING, ENUM, FIELDS ) \
      } \
      return fc::to_string(int64_t(elem)); \
    } \
    static std::string to_fc_string(int64_t i) { \
      return to_fc_string(ENUM(i)); \
    } \
    static ENUM from_int(int64_t i) { \
      ENUM e = ENUM(i); \
      switch( e ) \
      { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING_CASE, ENUM, FIELDS ) \
          break; \
        default: \
          fc::throw_bad_enum_cast( i, BOOST_PP_STRINGIZE(ENUM) ); \
      } \
      return e;\
    } \
    static ENUM from_string( const char* s ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING, ENUM, FIELDS ) \
        int64_t i = 0; \
        try \
        { \
           i = boost::lexical_cast<int64_t>(s); \
        } \
        catch( const boost::bad_lexical_cast& ) \
        { \
           fc::throw_bad_enum_cast( s, BOOST_PP_STRINGIZE(ENUM) ); \
        } \
        return from_int(i); \
    } \
    template< typename Visitor > \
    static void visit( Visitor& v ) \
    { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
    } \
};  \
template<> struct get_typename<ENUM>  { static const char* name()  { return BOOST_PP_STRINGIZE(ENUM);  } }; \
}

/*  Note: FC_REFLECT_ENUM previously defined this function, but I don't think it ever
 *        did what we expected it to do.  I've disabled it for now.
 *
 *  template<typename Visitor> \
 *  static inline void visit( const Visitor& v ) { \
 *      BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
 *  }\
 */

/**
 *  @def FC_REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes fc::reflector for TYPE where
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define FC_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type  is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
FC_OBJECT_REFLECTION_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_MEMBER_NAME, TYPE, MEMBERS ) \
} }

#define FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct get_typename<TYPE> { \
    static const char* name() { return BOOST_PP_STRINGIZE(TYPE); } \
  }; \
template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type  is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
FC_OBJECT_REFLECTION_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS) \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_TEMPLATE_MEMBER_NAME, (TEMPLATE_ARGS)(TYPE), MEMBERS ) \
} }

#define FC_REFLECT_DERIVED_NO_TYPENAME( TYPE, INHERITS, MEMBERS ) \
namespace fc { \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
FC_OBJECT_REFLECTION_DERIVED( TYPE, INHERITS, MEMBERS) \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_MEMBER_NAME, TYPE, MEMBERS ) \
} } // fc::member_names

/**
 *  @def FC_REFLECT(TYPE,MEMBERS)
 *  @brief Specializes fc::reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see FC_REFLECT_DERIVED
 */
#define FC_REFLECT( TYPE, MEMBERS ) \
   FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )


#define FC_REFLECT_TEMPLATE( TEMPLATE_ARGS, TYPE, MEMBERS ) \
    FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_REFLECT_EMPTY( TYPE ) \
    FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL )

#define FC_REFLECT_TYPENAME( TYPE ) \
namespace fc { \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
}

#define FC_INTERNAL_MEMBER_REFLECTOR(r, container, idx, member) \
   , BOOST_PP_SEQ_ENUM((fc::field_reflector<idx)(container)(decltype(container::member))(&container::member>))

/**
 *  @def FC_REFLECT_INTERNAL(TYPE,MEMBERS)
 *  @brief Reflects members of TYPE from within the class
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  This macro may be used from within the public interface of a class to reflect its members. Note that to complete
 *  the reflection, FC_COMPLETE_INTERNAL_REFLECTION(TYPE) must still be invoked from the global namespace.
 *
 *  Unfortunately, it is not possible to autogenerate an @ref object_reflection for types with internal reflection
 */
#define FC_REFLECT_INTERNAL( TYPE, MEMBERS ) \
   struct FC_internal_reflector { \
      using type = TYPE; \
      /* FC_INTERNAL_MEMBER_REFLECTION always has a leading comma, so pre-pack the list with a void and slice it */ \
      using members = typename fc::typelist::slice<fc::typelist::list<void \
         BOOST_PP_SEQ_FOR_EACH_I( FC_INTERNAL_MEMBER_REFLECTOR, TYPE, MEMBERS ) >, 1>; \
      FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, BOOST_PP_SEQ_NIL, MEMBERS ) \
   };

/**
 *  @def FC_COMPLETE_INTERNAL_REFLECTION(TYPE)
 *  @brief Complete reflection of a type with internal reflection definitions
 */
#define FC_COMPLETE_INTERNAL_REFLECTION( TYPE ) \
namespace fc { \
   template<> struct get_typename<TYPE> { \
      static const char* name() { return BOOST_PP_STRINGIZE(TYPE); } \
   }; \
   template<> struct reflector<TYPE> { \
      using type = TYPE; \
      using is_defined = std::true_type; \
      using native_members = TYPE::FC_internal_reflector::members; \
      using inherited_members = typelist::list<>; \
      using members = native_members; \
      using base_classes = typelist::list<>; \
      enum member_count_enum { \
         local_member_count = typelist::length<members>(), \
         total_member_count = local_member_count \
      }; \
      template<typename Visitor> \
      static inline void visit(const Visitor& v) { TYPE::FC_internal_reflector::visit(v); } \
   }; \
}
/**
 *  @def FC_COMPLETE_INTERNAL_REFLECTION_TEMPLATE(TYPE)
 *  @brief Complete reflection of a type with internal reflection definitions
 */
#define FC_COMPLETE_INTERNAL_REFLECTION_TEMPLATE( TEMPLATE_ARGS, TYPE ) \
namespace fc { \
   template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct get_typename<TYPE> { \
      static const char* name() { return BOOST_PP_STRINGIZE(TYPE); } \
   }; \
   template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> { \
      using type = TYPE; \
      using is_defined = std::true_type; \
      using native_members = typename TYPE::FC_internal_reflector::members; \
      using inherited_members = typelist::list<>; \
      using members = native_members; \
      using base_classes = typelist::list<>; \
      enum member_count_enum { \
         local_member_count = typelist::length<members>(), \
         total_member_count = local_member_count \
      }; \
      template<typename Visitor> \
      static inline void visit(const Visitor& v) { TYPE::FC_internal_reflector::visit(v); } \
   }; \
}
