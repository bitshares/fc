#include <fc/reflect/variant.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/io/json.hpp>

namespace fc { namespace rpc {

websocket_api_connection::~websocket_api_connection()
{
}

websocket_api_connection::websocket_api_connection( const std::shared_ptr<fc::http::websocket_connection>& c,
                                                    uint32_t max_depth )
   : api_connection(max_depth),_connection(c)
{
   FC_ASSERT( _connection, "A valid websocket connection is required" );
   _rpc_state.add_method( "call", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 3 && args[2].is_array() );
      api_id_type api_id;
      if( args[0].is_string() )
      {
         variant subresult = this->receive_call( 1, args[0].as_string() );
         api_id = subresult.as_uint64();
      }
      else
         api_id = args[0].as_uint64();

      return this->receive_call(
         api_id,
         args[1].as_string(),
         args[2].get_array() );
   } );

   _rpc_state.add_method( "notice", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_notice( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.add_method( "callback", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_callback( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.on_unhandled( [&]( const std::string& method_name, const variants& args )
   {
      return this->receive_call( 0, method_name, args );
   } );

   _connection->on_message_handler( [this]( const std::string& msg ){
       response reply = on_message(msg);
       if( _connection && ( reply.id || reply.result || reply.error || reply.jsonrpc ) )
          _connection->send_message( fc::json::to_string( reply, fc::json::stringify_large_ints_and_doubles,
                                                          _max_conversion_depth ) );
   } );
   _connection->on_http_handler( [this]( const std::string& msg ){
       response reply = on_message(msg);
       fc::http::reply result;
       if( reply.error )
       {
          if( reply.error->code == -32603 )
             result.status = fc::http::reply::InternalServerError;
          else if( reply.error->code <= -32600 )
             result.status = fc::http::reply::BadRequest;
       }
       if( reply.id || reply.result || reply.error || reply.jsonrpc )
          result.body_as_string = fc::json::to_string( reply, fc::json::stringify_large_ints_and_doubles,
                                                       _max_conversion_depth );
       else
          result.status = fc::http::reply::NoContent;

       return result;
   } );
   _connection->closed.connect( [this](){
      closed();
      _connection = nullptr;
   } );
}

variant websocket_api_connection::send_call(
   api_id_type api_id,
   string method_name,
   variants args /* = variants() */ )
{
   if( !_connection ) // defensive check
      return variant(); // TODO return an error?

   auto request = _rpc_state.start_remote_call( "call", { api_id, std::move(method_name), std::move(args) } );
   _connection->send_message( fc::json::to_string( fc::variant( request, _max_conversion_depth ),
                                                   fc::json::stringify_large_ints_and_doubles,
                                                   _max_conversion_depth ) );
   return request.done.get();
}

variant websocket_api_connection::send_callback(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   if( !_connection ) // defensive check
      return variant(); // TODO return an error?

   auto request = _rpc_state.start_remote_call( "callback", { callback_id, std::move(args) } );
   _connection->send_message( fc::json::to_string( fc::variant( request, _max_conversion_depth ),
                                                   fc::json::stringify_large_ints_and_doubles,
                                                   _max_conversion_depth ) );
   return request.done.get();
}

void websocket_api_connection::send_notice(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   if( !_connection ) // defensive check
      return;

   fc::rpc::request req{ optional<uint64_t>(), "notice", { callback_id, std::move(args) } };
   _connection->send_message( fc::json::to_string( fc::variant( req, _max_conversion_depth ),
                                                   fc::json::stringify_large_ints_and_doubles,
                                                   _max_conversion_depth ) );
}

response websocket_api_connection::on_message( const std::string& message )
{
   variant var;
   try
   {
      var = fc::json::from_string( message, fc::json::legacy_parser, _max_conversion_depth );
   }
   catch( const fc::exception& e )
   {
      return response( variant(), { -32700, "Invalid JSON message", variant( e, _max_conversion_depth ) }, "2.0" );
   }

   if( var.is_array() )
      return response( variant(), { -32600, "Batch requests not supported" }, "2.0" );

   if( !var.is_object() )
      return response( variant(), { -32600, "Invalid JSON request" }, "2.0" );

   variant_object var_obj = var.get_object();

   if( var_obj.contains( "id" )
       && !var_obj["id"].is_string() && !var_obj["id"].is_numeric() && !var_obj["id"].is_null() )
      return response( variant(), { -32600, "Invalid id" }, "2.0" );

   if( var_obj.contains( "method" ) && ( !var_obj["method"].is_string() || var_obj["method"].get_string() == "" ) )
      return response( variant(), { -32600, "Missing or invalid method" }, "2.0" );

   if( var_obj.contains( "jsonrpc" ) && ( !var_obj["jsonrpc"].is_string() || var_obj["jsonrpc"] != "2.0" ) )
      return response( variant(), { -32600, "Unsupported JSON-RPC version" }, "2.0" );

   if( var_obj.contains( "method" ) )
   {
      if( var_obj.contains( "params" ) && var_obj["params"].is_object() )
         return response( variant(), { -32602, "Named parameters not supported" }, "2.0" );

      if( var_obj.contains( "params" ) && !var_obj["params"].is_array() )
         return response( variant(), { -32600, "Invalid parameters" }, "2.0" );

      return on_request( std::move( var ) );
   }

   if( var_obj.contains( "result" ) || var_obj.contains("error") )
   {
      if( !var_obj.contains( "id" ) || ( var_obj["id"].is_null() && !var_obj.contains( "jsonrpc" ) ) )
         return response( variant(), { -32600, "Missing or invalid id" }, "2.0" );

      on_response( std::move( var ) );

      return response();
   }

   return response( variant(), { -32600, "Missing method or result or error" }, "2.0" );
}

void websocket_api_connection::on_response( const variant& var )
{
   _rpc_state.handle_reply( var.as<fc::rpc::response>(_max_conversion_depth) );
}

response websocket_api_connection::on_request( const variant& var )
{
   request call = var.as<fc::rpc::request>( _max_conversion_depth );
   if( var.get_object().contains( "id" ) )
      call.id = var.get_object()["id"]; // special handling for null id

   // null ID is valid in JSONRPC-2.0 but signals "no id" in JSONRPC-1.0
   bool has_id = call.id.valid() && ( call.jsonrpc.valid() || !call.id->is_null() );

   try
   {
#ifdef LOG_LONG_API
      auto start = time_point::now();
#endif

      auto result = _rpc_state.local_call( call.method, call.params );

#ifdef LOG_LONG_API
      auto end = time_point::now();

      if( end - start > fc::milliseconds( LOG_LONG_API_MAX_MS ) )
         elog( "API call execution time limit exceeded. method: ${m} params: ${p} time: ${t}",
               ("m",call.method)("p",call.params)("t", end - start) );
      else if( end - start > fc::milliseconds( LOG_LONG_API_WARN_MS ) )
         wlog( "API call execution time nearing limit. method: ${m} params: ${p} time: ${t}",
               ("m",call.method)("p",call.params)("t", end - start) );
#endif

      if( has_id )
         return response( call.id, result, call.jsonrpc );
   }
   catch ( const fc::method_not_found_exception& e )
   {
      if( has_id )
         return response( call.id, error_object{ -32601, "Method not found",
                          variant( (fc::exception) e, _max_conversion_depth ) }, call.jsonrpc );
   }
   catch ( const fc::exception& e )
   {
      if( has_id )
         return response( call.id, error_object{ e.code(), "Execution error", variant( e, _max_conversion_depth ) },
                          call.jsonrpc );
   }
   catch ( const std::exception& e )
   {
      elog( "Internal error - ${e}", ("e",e.what()) );
      return response( call.id, error_object{ -32603, "Internal error", variant( e.what(), _max_conversion_depth ) },
                       call.jsonrpc );
   }
   catch ( ... )
   {
      elog( "Internal error while processing RPC request" );
      throw;
   }
   return response();
}

} } // namespace fc::rpc
