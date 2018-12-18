#pragma once
#include <fc/variant.hpp>
#include <fc/optional.hpp>
#include <fc/api.hpp>
#include <fc/any.hpp>
#include <memory>
#include <vector>
#include <functional>
#include <utility>
#include <fc/signals.hpp>

namespace fc {
   class api_connection;

   namespace detail {
      template<typename Signature>
      class callback_functor
      {
         public:
            typedef typename std::function<Signature>::result_type result_type;

            callback_functor( std::weak_ptr< fc::api_connection > con, uint64_t id )
            :_callback_id(id),_api_connection(con){}

            template<typename... Args> 
            result_type operator()( Args... args )const;

         private:
            uint64_t _callback_id;
            std::weak_ptr< fc::api_connection > _api_connection;
      };

      template<typename R, typename Arg0, typename ... Args>
      std::function<R(Args...)> bind_first_arg( const std::function<R(Arg0,Args...)>& f, Arg0 a0 )
      {
         return [=]( Args... args ) { return f( a0, args... ); };
      }
      template<typename R>
      R call_generic( const std::function<R()>& f, variants::const_iterator a0, variants::const_iterator e, uint32_t max_depth = 1 )
      {
         return f();
      }

      template<typename R, typename Arg0, typename ... Args>
      R call_generic( const std::function<R(Arg0,Args...)>& f, variants::const_iterator a0, variants::const_iterator e, uint32_t max_depth )
      {
         FC_ASSERT( a0 != e );
         FC_ASSERT( max_depth > 0, "Recursion depth exceeded!" );
         return call_generic<R,Args...>( bind_first_arg<R,Arg0,Args...>( f, a0->as< typename std::decay<Arg0>::type >( max_depth - 1 ) ), a0+1, e, max_depth - 1 );
      }

      template<typename R, typename ... Args>
      std::function<variant(const fc::variants&, uint32_t)> to_generic( const std::function<R(Args...)>& f )
      {
         return [=]( const variants& args, uint32_t max_depth ) {
            FC_ASSERT( max_depth > 0, "Recursion depth exceeded!" );
            return variant( call_generic( f, args.begin(), args.end(), max_depth - 1 ), max_depth - 1 );
         };
      }

      template<typename ... Args>
      std::function<variant(const fc::variants&, uint32_t)> to_generic( const std::function<void(Args...)>& f )
      {
         return [=]( const variants& args, uint32_t max_depth ) {
            FC_ASSERT( max_depth > 0, "Recursion depth exceeded!" );
            call_generic( f, args.begin(), args.end(), max_depth - 1 );
            return variant();
         };
      }

      /**
       * If api<T> is returned from a remote method, the API is eagerly bound to api<T> of
       * the correct type in api_visitor::from_variant().  This binding [1] needs a reference
       * to the api_connection, which is made available to from_variant() as a parameter.
       *
       * However, in the case of a remote method which returns api_base which can subsequently
       * be cast by the caller with as<T>, we need to keep track of the connection because
       * the binding is done later (when the client code actually calls as<T>).
       *
       * [1] The binding actually happens in get_remote_api().
       */
      class any_api : public api_base
      {
         public:
            any_api( api_id_type api_id, const std::shared_ptr<fc::api_connection>& con )
               : _api_id(api_id), _api_connection(con) {}

            virtual uint64_t get_handle()const override
            {  return _api_id;    }

            virtual api_id_type register_api( api_connection& conn )const override
            {  FC_ASSERT( false ); return api_id_type(); }

            api_id_type                         _api_id;
            std::weak_ptr<fc::api_connection>   _api_connection;
      };

   } // namespace detail

   class generic_api;

   template<typename Result, typename ...Args>
   class to_eneric
   {
   public:
       static std::function<variant(const fc::variants&)> generate(
           const std::function<Result(Args...)>& f,
           const std::weak_ptr<fc::api_connection>& api_conn,
           generic_api& _api);
   };

   template<typename ...Args>
   class to_eneric<void, Args...>
   {
   public:
       static std::function<variant(const fc::variants&)> generate(
           const std::function<void(Args...)>& f,
           const std::weak_ptr<fc::api_connection>& api_conn,
           generic_api& _api);
   };

   template<typename ...Args>
   class to_eneric<fc::api_ptr, Args...>
   {
   public:
       static std::function<variant(const fc::variants&)> generate(
           const std::function<fc::api_ptr(Args...)>& f,
           const std::weak_ptr<fc::api_connection>& api_conn,
           generic_api& _api);
   };

   template<class Interface, class Adaptor, typename ...Args>
   class to_eneric< api<Interface, Adaptor>, Args...>
   {
   public:
       static std::function<variant(const fc::variants&)> generate(
           const std::function<api<Interface, Adaptor>(Args...)>& f,
           const std::weak_ptr<fc::api_connection>& api_conn,
           generic_api& _api);
   };
   ;
   
   template<class Interface, class Adaptor, typename ...Args>
   class to_eneric< fc::optional<api<Interface, Adaptor>>, Args...>
   {
   public:
       static std::function<variant(const fc::variants&)> generate(
           const std::function<fc::optional<api<Interface, Adaptor>>(Args...)>& f,
           const std::weak_ptr<fc::api_connection>& api_conn,
           generic_api& _api);
   };

   template<typename R, typename Arg0, typename ... Args>
   class bind_first_arg
   {
   public:
       static std::function<R(Args...)> bind(const std::function<R(Arg0, Args...)>& f, Arg0 a0)
       {
           return [=](Args... args) { return f(a0, args...); };
       }
   };

   template<class R, typename ...Args>
   class call_generic
   {
   public:

       static R call(
           std::weak_ptr< fc::api_connection >,
           const std::function<R(Args...)>& f, 
           variants::const_iterator a0, 
           variants::const_iterator e, 
           uint32_t max_depth)
       {
           return _call(f, a0, e, max_depth);
       }

   private:
       static R _call(const std::function<R()>& f, variants::const_iterator a0, variants::const_iterator e, uint32_t max_depth = 1)
       {
           return f();
       }

       template<typename Arg0, typename ...LeftArgs>
       static R _call(const std::function<R(Arg0, LeftArgs...)>& f, variants::const_iterator a0, variants::const_iterator e, uint32_t max_depth)
       {
           return  _call(
               bind_first_arg<R, Arg0, LeftArgs...>::bind(f, a0->as< typename std::decay<Arg0>::type >(max_depth - 1)),
               a0 + 1,
               e,
               max_depth - 1);
       }
   };

   template<class R, typename Signature, typename ...Args>
   class call_generic<R, const std::function<Signature>&, Args...>
   {
   public:
       static R call(
           std::weak_ptr< fc::api_connection > con,
           const std::function<R(const std::function<Signature>&, Args...)>& f,
           variants::const_iterator a0,
           variants::const_iterator e,
           uint32_t max_depth)
       {
           FC_ASSERT(a0 != e, "too few arguments passed to method");
           FC_ASSERT(max_depth > 0, "Recursion depth exceeded!");
           detail::callback_functor<Signature> arg0(con, a0->as<uint64_t>(1));
           return call_generic<R, Args...>::call(
               con, 
               bind_first_arg<R, std::function<Signature>, Args...>::bind(f, std::function<Signature>(arg0)), 
               a0 + 1, 
               e, 
               max_depth - 1);
       }
   };

   template<class R, typename Signature, typename ...Args>
   class call_generic<R, std::function<Signature>, Args...>
   {
   public:
       static R call(
           std::weak_ptr< fc::api_connection > con,
           const std::function<R(std::function<Signature>, Args...)>& f,
           variants::const_iterator a0,
           variants::const_iterator e,
           uint32_t max_depth)
       {
           FC_ASSERT(a0 != e, "too few arguments passed to method");
           FC_ASSERT(max_depth > 0, "Recursion depth exceeded!");
           detail::callback_functor<Signature> arg0(con, a0->as<uint64_t>(1));
           return call_generic<R, Args...>::call(
               con,
               bind_first_arg<R, const std::function<Signature>&, Args...>::bind(f, arg0), 
               a0 + 1, 
               e, 
               max_depth - 1);
       }
   };

   template<class R>
   class call_generic<R, void>
   {
   public:
       static R call(
           std::weak_ptr< fc::api_connection > con,
           const std::function<R()>& f,
           variants::const_iterator a0,
           variants::const_iterator e,
           uint32_t max_depth)
       {
           return f();
       }
   };

   class generic_api
   {
      public:
         template<typename Api>
         generic_api( const Api& a, const std::shared_ptr<fc::api_connection>& c );

         generic_api( const generic_api& cpy ) = delete;

         variant call( const string& name, const variants& args )
         {
            auto itr = _by_name.find(name);
            FC_ASSERT( itr != _by_name.end(), "no method with name '${name}'", ("name",name)("api",_by_name) );
            return call( itr->second, args );
         }

         variant call( uint32_t method_id, const variants& args )
         {
            FC_ASSERT( method_id < _methods.size() );
            return _methods[method_id](args);
         }

         std::weak_ptr< fc::api_connection > get_connection()
         {
            return _api_connection;
         }

         std::vector<std::string> get_method_names()const
         {
            std::vector<std::string> result;
            result.reserve( _by_name.size() );
            for( auto& m : _by_name ) result.push_back(m.first);
            return result;
         }

      private:
         friend struct api_visitor;

         template<typename R, typename Arg0, typename ... Args>
         std::function<R(Args...)> bind_first_arg( const std::function<R(Arg0,Args...)>& f, Arg0 a0 )const
         {
            return [=]( Args... args ) { return f( a0, args... ); };
         }

         struct api_visitor
         {
            api_visitor( generic_api& a, const std::weak_ptr<fc::api_connection>& s ):_api(a),_api_con(s){ }

            template<typename Result, typename... Args>
            void operator()( const char* name, std::function<Result(Args...)>& memb )const {
               _api._methods.emplace_back(to_eneric<Result, Args...>::generate(memb, _api_con, _api));
               _api._by_name[name] = _api._methods.size() - 1;
            }

            generic_api& _api;
            const std::weak_ptr<fc::api_connection>& _api_con;
         };


         std::weak_ptr<fc::api_connection>                       _api_connection;
         fc::any                                                 _api;
         std::map< std::string, uint32_t >                       _by_name;
         std::vector< std::function<variant(const variants&)> >  _methods;
   }; // class generic_api



   class api_connection : public std::enable_shared_from_this<fc::api_connection>
   {
      public:
         api_connection(uint32_t max_depth):_max_conversion_depth(max_depth){}
         virtual ~api_connection(){};


         template<typename T>
         api<T> get_remote_api( api_id_type api_id = 0 )
         {
            api<T> result;
            result->visit( api_visitor(  api_id, this->shared_from_this() ) );
            return result;
         }
  
         /** makes calls to the remote server */
         virtual variant send_call( api_id_type api_id, string method_name, variants args = variants() ) = 0;
         virtual variant send_callback( uint64_t callback_id, variants args = variants() ) = 0;
         virtual void    send_notice( uint64_t callback_id, variants args = variants() ) = 0;

         variant receive_call( api_id_type api_id, const string& method_name, const variants& args = variants() )const
         {
            FC_ASSERT( _local_apis.size() > api_id );
            return _local_apis[api_id]->call( method_name, args );
         }
         variant receive_callback( uint64_t callback_id,  const variants& args = variants() )const
         {
            FC_ASSERT( _local_callbacks.size() > callback_id );
            return _local_callbacks[callback_id]( args, _max_conversion_depth );
         }
         void receive_notice( uint64_t callback_id,  const variants& args = variants() )const
         {
            FC_ASSERT( _local_callbacks.size() > callback_id );
            _local_callbacks[callback_id]( args, _max_conversion_depth );
         }

         template<typename Interface>
         api_id_type register_api( const Interface& a )
         {
            auto handle = a.get_handle();
            auto itr = _handle_to_id.find(handle);
            if( itr != _handle_to_id.end() ) return itr->second;

            _local_apis.push_back( std::unique_ptr<generic_api>( new generic_api(a, shared_from_this() ) ) );
            _handle_to_id[handle] = _local_apis.size() - 1;
            return _local_apis.size() - 1;
         }

         template<typename Signature>
         uint64_t register_callback( const std::function<Signature>& cb )
         {
            _local_callbacks.push_back( detail::to_generic( cb ) );
            return _local_callbacks.size() - 1;
         }

         std::vector<std::string> get_method_names( api_id_type local_api_id = 0 )const { return _local_apis[local_api_id]->get_method_names(); }

         fc::signal<void()> closed;
         const uint32_t     _max_conversion_depth; // for nested structures, json, variant etc.
      private:
         std::vector< std::unique_ptr<generic_api> >                      _local_apis;
         std::map< uint64_t, api_id_type >                                _handle_to_id;
         std::vector< std::function<variant(const variants&, uint32_t)> > _local_callbacks;


         struct api_visitor
         {
            uint32_t                            _api_id;
            std::shared_ptr<fc::api_connection> _connection;

            api_visitor( uint32_t api_id, std::shared_ptr<fc::api_connection> con )
            :_api_id(api_id),_connection(std::move(con))
            {
            }

            api_visitor() = delete;

            template<typename Result>
            static Result from_variant( const variant& v, Result*, const std::shared_ptr<fc::api_connection>&, uint32_t max_depth )
            {
               return v.as<Result>( max_depth );
            }

            template<typename ResultInterface>
            static fc::api<ResultInterface> from_variant( const variant& v, 
                                                          fc::api<ResultInterface>* /*used for template deduction*/,
                                                          const std::shared_ptr<fc::api_connection>&  con,
                                                          uint32_t max_depth = 1
                                                        )
            {
               return con->get_remote_api<ResultInterface>( v.as_uint64() );
            }

            static fc::api_ptr from_variant(
               const variant& v,
               fc::api_ptr* /* used for template deduction */,
               const std::shared_ptr<fc::api_connection>&  con,
               uint32_t max_depth = 1
            )
            {
               if( v.is_null() )
                  return fc::api_ptr();
               return fc::api_ptr( new detail::any_api( v.as_uint64(), con ) );
            }

            template<typename T>
            static fc::variant convert_callbacks( const std::shared_ptr<fc::api_connection>& con, const T& v )
            { 
               return fc::variant( v, con->_max_conversion_depth );
            }

            template<typename Signature>
            static fc::variant convert_callbacks( const std::shared_ptr<fc::api_connection>& con, const std::function<Signature>& v ) 
            { 
               return con->register_callback( v ); 
            }

            template<typename Result, typename... Args>
            void operator()( const char* name, std::function<Result(Args...)>& memb )const 
            {
                auto con   = _connection;
                auto api_id = _api_id;
                memb = [con,api_id,name]( Args... args ) {
                    auto var_result = con->send_call( api_id, name, { convert_callbacks(con,args)...} );
                    return from_variant( var_result, (Result*)nullptr, con, con->_max_conversion_depth );
                };
            }
            template<typename... Args>
            void operator()( const char* name, std::function<void(Args...)>& memb )const 
            {
                auto con   = _connection;
                auto api_id = _api_id;
                memb = [con,api_id,name]( Args... args ) {
                   con->send_call( api_id, name, { convert_callbacks(con,args)...} );
                };
            }
         };
   };
   
   template<typename Result, typename ...Args>
   std::function<variant(const fc::variants&)> to_eneric<Result, Args...>::generate(
       const std::function<Result(Args...)>& f,
       const std::weak_ptr<fc::api_connection>& api_con,
       generic_api& _api)
   {
       auto con = api_con.lock();
       FC_ASSERT(con, "not connected");
       uint32_t max_depth = con->_max_conversion_depth;
       auto gapi = &_api;

       return [=](const variants& args) {
           return variant(call_generic<Result, Args...>::call(gapi->get_connection(), f, args.begin(), args.end(), max_depth), max_depth);
       };
   }
   
   template<typename ...Args>
   std::function<variant(const fc::variants&)> to_eneric<void, Args...>::generate(
       const std::function<void(Args...)>& f,
       const std::weak_ptr<fc::api_connection>& api_con,
       generic_api& _api)
   {
       auto con = api_con.lock();
       FC_ASSERT(con, "not connected");
       uint32_t max_depth = con->_max_conversion_depth;
       auto gapi = &_api;

       return [=](const variants& args) {
           call_generic<void, Args...>::call(gapi->get_connection(), f, args.begin(), args.end(), max_depth );
           return variant();
       };
   }
   
   template<typename ...Args>
   std::function<variant(const fc::variants&)> to_eneric<fc::api_ptr, Args...>::generate(
       const std::function<fc::api_ptr(Args...)>& f,
       const std::weak_ptr<fc::api_connection>& api_con,
       generic_api& _api)
   {
       auto gapi = &_api;

       return [=](const variants& args) {
           auto con = api_con.lock();
           FC_ASSERT(con, "not connected");

           auto api_result = call_generic<fc::api_ptr, Args...>::call(gapi->get_connection(), f, args.begin(), args.end(), con->_max_conversion_depth);
           if (!api_result)
               return variant();
           return api_result->register_api(*con);
       };
   }
   
   template<class Interface, class Adaptor, typename ...Args>
   std::function<variant(const fc::variants&)> to_eneric< api<Interface, Adaptor>, Args...>::generate(
       const std::function<api<Interface, Adaptor>(Args...)>& f,
       const std::weak_ptr<fc::api_connection>& api_con,
       generic_api& _api)
   {
       auto gapi = &_api;

       return [=](const variants& args) {
           auto con = api_con.lock();
           FC_ASSERT(con, "not connected");

           auto api_result = call_generic<api<Interface, Adaptor>, Args...>::call(gapi->get_connection(), f, args.begin(), args.end(), con->_max_conversion_depth);
           return con->register_api(api_result);
       };
   }

   template<class Interface, class Adaptor, typename ...Args>
   std::function<variant(const fc::variants&)> to_eneric< fc::optional<api<Interface, Adaptor>>, Args...>::generate(
        const std::function<fc::optional<api<Interface, Adaptor>>(Args...)>& f,
        const std::weak_ptr<fc::api_connection>& api_con,
        generic_api& _api)
   {
       auto gapi = &_api;

       return [=](const variants& args) {
           auto con = api_con.lock();
           FC_ASSERT(con, "not connected");

           auto api_result = call_generic<fc::optional<api<Interface, Adaptor>>, Args...>::call(gapi->get_connection(), f, args.begin(), args.end(), con->_max_conversion_depth);
           if (api_result)
               return con->register_api(*api_result);
           return variant();
       };
   }

   class local_api_connection : public api_connection
   {
      public:
         local_api_connection( uint32_t max_depth ) : api_connection(max_depth){}
         ~local_api_connection(){}

         /** makes calls to the remote server */
         virtual variant send_call( api_id_type api_id, string method_name, variants args = variants() ) override
         {
            FC_ASSERT( _remote_connection );
            return _remote_connection->receive_call( api_id, method_name, std::move(args) );
         }
         virtual variant send_callback( uint64_t callback_id, variants args = variants() ) override
         {
            FC_ASSERT( _remote_connection );
            return _remote_connection->receive_callback( callback_id, args );
         }
         virtual void send_notice( uint64_t callback_id, variants args = variants() ) override
         {
            FC_ASSERT( _remote_connection );
            _remote_connection->receive_notice( callback_id, args );
         }


         void  set_remote_connection( const std::shared_ptr<fc::api_connection>& rc )
         {
            FC_ASSERT( !_remote_connection );
            FC_ASSERT( rc != this->shared_from_this() );
            _remote_connection = rc;
         }
         const std::shared_ptr<fc::api_connection>& remote_connection()const  { return _remote_connection; }

         std::shared_ptr<fc::api_connection>    _remote_connection;
   };

   template<typename Api>
   generic_api::generic_api( const Api& a, const std::shared_ptr<fc::api_connection>& c )
   :_api_connection(c),_api(a)
   {
      boost::any_cast<const Api&>(a)->visit( api_visitor( *this, c ) );
   }

   /**
    * It is slightly unclean tight coupling to have this method in the api class.
    * It breaks encapsulation by requiring an api class method to have a pointer
    * to an api_connection.  The reason this is necessary is we have a goal of being
    * able to call register_api() on an api<T> through its base class api_base.  But
    * register_api() must know the template parameters!
    *
    * The only reasonable way to achieve the goal is to implement register_api()
    * as a method in api<T> (which obviously knows the template parameter T),
    * then make the implementation accessible through the base class (by making
    * it a pure virtual method in the base class which is overridden by the subclass's
    * implementation).
    */
   template< typename Interface, typename Transform >
   api_id_type api< Interface, Transform >::register_api( api_connection& conn )const
   {
      return conn.register_api( *this );
   }

   template< typename T >
   api<T> api_base::as()
   {
      // TODO:  this method should probably be const (if it is not too hard)
      api<T>* maybe_requested_type = dynamic_cast< api<T>* >(this);
      if( maybe_requested_type != nullptr )
         return *maybe_requested_type;

      detail::any_api* maybe_any = dynamic_cast< detail::any_api* >(this);
      FC_ASSERT( maybe_any != nullptr );
      std::shared_ptr< api_connection > api_conn = maybe_any->_api_connection.lock();
      FC_ASSERT( api_conn );
      return api_conn->get_remote_api<T>( maybe_any->_api_id );
   }

   namespace detail {
      template<typename Signature>
      template<typename... Args> 
      typename callback_functor<Signature>::result_type callback_functor<Signature>::operator()( Args... args )const
      {
         std::shared_ptr< fc::api_connection > locked = _api_connection.lock();
         // TODO:  make new exception type for this instead of recycling eof_exception
         if( !locked )
            throw fc::eof_exception();
         locked->send_callback( _callback_id, fc::variants{ args... } ).template as< result_type >();
      }


      template<typename... Args>
      class callback_functor<void(Args...)>
      {
         public:
          typedef void result_type;

          callback_functor( std::weak_ptr< fc::api_connection > con, uint64_t id )
          :_callback_id(id),_api_connection(con){}

          void operator()( Args... args )const
          {
             std::shared_ptr< fc::api_connection > locked = _api_connection.lock();
             // TODO:  make new exception type for this instead of recycling eof_exception
             if( !locked )
                throw fc::eof_exception();
             locked->send_notice( _callback_id, fc::variants{ args... } );
          }

         private:
          uint64_t _callback_id;
          std::weak_ptr< fc::api_connection > _api_connection;
      };
   } // namespace detail

} // fc
