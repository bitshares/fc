#pragma once 

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(push) 
#pragma warning(disable:4996) 
#endif 

#include <boost/signals2/signal.hpp>

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(pop) 
#endif 

namespace fc {
#if !defined(BOOST_NO_TEMPLATE_ALIASES) 
   template<typename T>
   using signal = boost::signals2::signal<T>;

   using scoped_connection = boost::signals2::scoped_connection;
#else
  /** Workaround for missing Template Aliases feature in the VS 2012.
      \warning Class defined below cannot have defined constructor (even base class has it)
      since it is impossible to reference directly template class arguments outside this class.
      This code will work until someone will use non-default constructor as it is defined in
      boost::signals2::signal.
  */
  template <class T>
  class signal : public boost::signals2::signal<T>
    {
    public:
    };
#endif

} 
