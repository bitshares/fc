#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/iostream.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/sstream.hpp>
#include <fc/log/logger.hpp>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/filesystem/fstream.hpp>

namespace fc
{
    // forward declarations of provided functions
    template<typename T, json::parse_type parser_type> variant variant_from_stream( T& in, uint32_t max_depth );
    template<typename T> char parseEscape( T& in );
    template<typename T> std::string stringFromStream( T& in );
    template<typename T> bool skip_white_space( T& in );
    template<typename T> std::string stringFromToken( T& in );
    template<typename T> variant_object objectFromStreamBase( T& in, std::function<std::string(T&)>& get_key, std::function<variant(T&)>& get_value );
    template<typename T, json::parse_type parser_type> variant_object objectFromStream( T& in, uint32_t max_depth );
    template<typename T> variants arrayFromStreamBase( T& in, std::function<variant(T&)>& get_value );
    template<typename T, json::parse_type parser_type> variants arrayFromStream( T& in, uint32_t max_depth );
    template<typename T, json::parse_type parser_type> variant number_from_stream( T& in );
    template<typename T> variant token_from_stream( T& in );
    void escape_string( const string& str, ostream& os );
    template<typename T> void to_stream( T& os, const variants& a, json::output_formatting format, uint32_t max_depth );
    template<typename T> void to_stream( T& os, const variant_object& o, json::output_formatting format, uint32_t max_depth );
    template<typename T> void to_stream( T& os, const variant& v, json::output_formatting format, uint32_t max_depth );
    std::string pretty_print( const std::string& v, uint8_t indent );
}

#if __cplusplus > 201402L
#define FALLTHROUGH      [[fallthrough]];
#else
#define FALLTHROUGH
#endif

#include <fc/io/json_relaxed.hpp>

namespace fc
{
   template<typename T>
   char parseEscape( T& in )
   {
      if( in.peek() == '\\' )
      {
         try {
            in.get();
            switch( in.peek() )
            {
               case 't':
                  in.get();
                  return '\t';
               case 'n':
                  in.get();
                  return '\n';
               case 'r':
                  in.get();
                  return '\r';
               case '\\':
                  in.get();
                  return '\\';
               default:
                  return in.get();
            }
         } FC_RETHROW_EXCEPTIONS( info, "Stream ended with '\\'" );
      }
	    FC_THROW_EXCEPTION( parse_error_exception, "Expected '\\'"  );
   }

   template<typename T>
   bool skip_white_space( T& in )
   {
       bool skipped = false;
       while( true )
       {
          switch( in.peek() )
          {
             case ' ':
             case '\t':
             case '\n':
             case '\r':
                skipped = true;
                in.get();
                break;
             default:
                return skipped;
          }
       }
   }

   template<typename T>
   std::string stringFromStream( T& in )
   {
      fc::stringstream token;
      try
      {
         char c = in.peek();

         if( c != '"' )
            FC_THROW_EXCEPTION( parse_error_exception,
                                            "Expected '\"' but read '${char}'",
                                            ("char", string(&c, (&c) + 1) ) );
         in.get();
         while( true )
         {

            switch( c = in.peek() )
            {
               case '\\':
                  token << parseEscape( in );
                  break;
               case 0x04:
                  FC_THROW_EXCEPTION( parse_error_exception, "EOF before closing '\"' in string '${token}'",
                                                   ("token", token.str() ) );
               case '"':
                  in.get();
                  return token.str();
               default:
                  token << c;
                  in.get();
            }
         }
         FC_THROW_EXCEPTION( parse_error_exception, "EOF before closing '\"' in string '${token}'",
                                          ("token", token.str() ) );
       } FC_RETHROW_EXCEPTIONS( warn, "while parsing token '${token}'",
                                          ("token", token.str() ) );
   }
   template<typename T>
   std::string stringFromToken( T& in )
   {
      fc::stringstream token;
      try
      {
         char c = in.peek();

         while( true )
         {
            switch( c = in.peek() )
            {
               case '\\':
                  token << parseEscape( in );
                  break;
               case '\t':
               case ' ':
               case '\0':
               case '\n':
                  in.get();
                  return token.str();
               default:
                if( isalnum( c ) || c == '_' || c == '-' || c == '.' || c == ':' || c == '/' )
                {
                  token << c;
                  in.get();
                }
                else return token.str();
            }
         }
         return token.str();
      }
      catch( const fc::eof_exception& eof )
      {
         return token.str();
      }
      catch (const std::ios_base::failure&)
      {
         return token.str();
      }

      FC_RETHROW_EXCEPTIONS( warn, "while parsing token '${token}'",
                                          ("token", token.str() ) );
   }

   template<typename T>
   variant_object objectFromStreamBase( T& in, std::function<std::string(T&)>& get_key, std::function<variant(T&)>& get_value )
   {
      mutable_variant_object obj;
      try
      {
         char c = in.peek();
         if( c != '{' )
            FC_THROW_EXCEPTION( parse_error_exception,
                                     "Expected '{', but read '${char}'",
                                     ("char",string(&c, &c + 1)) );
         in.get();
         while( in.peek() != '}' )
         {
            if( in.peek() == ',' )
            {
               in.get();
               continue;
            }
            if( skip_white_space(in) ) continue;
            string key = get_key( in );
            skip_white_space(in);
            if( in.peek() != ':' )
            {
               FC_THROW_EXCEPTION( parse_error_exception, "Expected ':' after key \"${key}\"",
                                        ("key", key) );
            }
            in.get();
            auto val = get_value( in );

            obj(std::move(key),std::move(val));
         }
         if( in.peek() == '}' )
         {
            in.get();
            return obj;
         }
         FC_THROW_EXCEPTION( parse_error_exception, "Expected '}' after ${variant}", ("variant", obj ) );
      }
      catch( const fc::eof_exception& e )
      {
         FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF: ${e}", ("e", e.to_detail_string() ) );
      }
      catch( const std::ios_base::failure& e )
      {
         FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF: ${e}", ("e", e.what() ) );
      } FC_RETHROW_EXCEPTIONS( warn, "Error parsing object" );
   }

   template<typename T, json::parse_type parser_type>
   variant_object objectFromStream( T& in, uint32_t max_depth )
   {
      std::function<std::string(T&)> get_key = []( T& in ){ return stringFromStream( in ); };
      std::function<variant(T&)> get_value = [max_depth]( T& in ){ return variant_from_stream<T, parser_type>( in, max_depth ); };
      return objectFromStreamBase<T>( in, get_key, get_value );
   }

   template<typename T>
   variants arrayFromStreamBase( T& in, std::function<variant(T&)>& get_value  )
   {
      variants ar;
      try
      {
        if( in.peek() != '[' )
           FC_THROW_EXCEPTION( parse_error_exception, "Expected '['" );
        in.get();

        while( in.peek() != ']' )
        {
           if( in.peek() == ',' )
           {
              in.get();
              continue;
           }
           if( skip_white_space(in) ) continue;
           ar.push_back( get_value(in) );
        }
        if( in.peek() != ']' )
           FC_THROW_EXCEPTION( parse_error_exception, "Expected ']' after parsing ${variant}",
                                    ("variant", ar) );

        in.get();
      } FC_RETHROW_EXCEPTIONS( warn, "Attempting to parse array ${array}",
                                         ("array", ar ) );
      return ar;
   }

   template<typename T, json::parse_type parser_type>
   variants arrayFromStream( T& in, uint32_t max_depth )
   {
      std::function<variant(T&)> get_value = [max_depth]( T& in ){ return variant_from_stream<T, parser_type>( in, max_depth ); };
      return arrayFromStreamBase<T>( in, get_value );
   }

   template<typename T, json::parse_type parser_type>
   variant number_from_stream( T& in )
   {
      fc::stringstream ss;

      bool  dot = false;
      bool  neg = false;
      if( in.peek() == '-')
      {
        neg = true;
        ss.put( in.get() );
      }
      bool done = false;

      try
      {
        char c;
        while((c = in.peek()) && !done)
        {

          switch( c )
          {
              case '.':
                 if (dot)
                    FC_THROW_EXCEPTION(parse_error_exception, "Can't parse a number with two decimal places");
                 dot = true;
                 FALLTHROUGH
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                 ss.put( in.get() );
                 break;
              default:
                 if( isalnum( c ) )
                 {
                    return ss.str() + stringFromToken( in );
                 }
                done = true;
                break;
          }
        }
      }
      catch (fc::eof_exception&)
      { // EOF ends the loop
      }
      catch (const std::ios_base::failure&)
      { // read error ends the loop
      }
      std::string str = ss.str();
      if (str == "-." || str == "." || str == "-") // check the obviously wrong things we could have encountered
        FC_THROW_EXCEPTION(parse_error_exception, "Can't parse token \"${token}\" as a JSON numeric constant", ("token", str));
      if( dot )
        return
#ifdef WITH_EXOTIC_JSON_PARSERS
              parser_type == json::legacy_parser_with_string_doubles ? variant(str) :
#endif
                  variant(to_double(str));
      if( neg )
        return to_int64(str);
      return to_uint64(str);
   }
   template<typename T>
   variant token_from_stream( T& in )
   {
      std::stringstream ss;
      ss.exceptions( std::ifstream::badbit );
      bool received_eof = false;
      bool done = false;

      try
      {
        char c;
        while((c = in.peek()) && !done)
        {
           switch( c )
           {
              case 'n':
              case 'u':
              case 'l':
              case 't':
              case 'r':
              case 'e':
              case 'f':
              case 'a':
              case 's':
                 ss.put( in.get() );
                 break;
              default:
                 done = true;
                 break;
           }
        }
      }
      catch (fc::eof_exception&)
      {
        received_eof = true;
      }
      catch (const std::ios_base::failure&)
      {
        received_eof = true;
      }

      // we can get here either by processing a delimiter as in "null,"
      // an EOF like "null<EOF>", or an invalid token like "nullZ"
      std::string str = ss.str();
      if( str == "null" )
        return variant();
      if( str == "true" )
        return true;
      if( str == "false" ) 
        return false;
      else
      {
        if (received_eof)
        {
          if (str.empty())
            FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF" );
          else
            return str;
        }
        else
        {
          // if we've reached this point, we've either seen a partial
          // token ("tru<EOF>") or something our simple parser couldn't
          // make out ("falfe")
          // A strict JSON parser would signal this as an error, but we
          // will just treat the malformed token as an un-quoted string.
          return str + stringFromToken(in);
        }
      }
   }


   template<typename T, json::parse_type parser_type>
   variant variant_from_stream( T& in, uint32_t max_depth )
   {
      if( max_depth == 0 )
          FC_THROW_EXCEPTION( parse_error_exception, "Too many nested items in JSON input!" );
      skip_white_space(in);
      signed char c = in.peek();
      switch( c )
      {
         case '"':
            return stringFromStream( in );
         case '{':
            return objectFromStream<T, parser_type>( in, max_depth - 1 );
         case '[':
            return arrayFromStream<T, parser_type>( in, max_depth - 1 );
         case '-':
         case '.':
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            return number_from_stream<T, parser_type>( in );
         // null, true, false, or 'warning' / string
         case 'n':
         case 't':
         case 'f':
            return token_from_stream( in );
         case 0x04: // ^D end of transmission
         case EOF:
            FC_THROW_EXCEPTION( eof_exception, "unexpected end of file" );
         case 0:
            if( parser_type == fc::json::broken_nul_parser )
               return variant();
            FALLTHROUGH
         default:
            FC_THROW_EXCEPTION( parse_error_exception, "Unexpected char '${c}' in \"${s}\"",
                                ("c", c)("s", stringFromToken(in)) );
      }
  }

   variant json::from_string( const std::string& utf8_str, parse_type ptype, uint32_t max_depth )
   { try {
      fc::istream_ptr in( new fc::stringstream( utf8_str ) );
      fc::buffered_istream bin( in );
      return from_stream( bin, ptype, max_depth );
   } FC_RETHROW_EXCEPTIONS( warn, "", ("str",utf8_str) ) }

   variants json::variants_from_string( const std::string& utf8_str, parse_type ptype, uint32_t max_depth )
   {
      variants result;
      try {
         fc::stringstream in( utf8_str );
         while( true )
            result.push_back(json_relaxed::variant_from_stream<fc::stringstream, false>( in, max_depth ));
      } catch ( const fc::eof_exception& ) {
         return result;
      } FC_RETHROW_EXCEPTIONS( warn, "", ("str",utf8_str) )
   }

   /**
    *  Convert '\t', '\a', '\n', '\\' and '"'  to "\t\a\n\\\""
    *
    *  All other characters are printed as UTF8.
    */
   void escape_string( const string& str, ostream& os )
   {
      os << '"';
      for( auto itr = str.begin(); itr != str.end(); ++itr )
      {
         switch( *itr )
         {
            case '\b':        // \x08
               os << "\\b";
               break;
            case '\f':        // \x0c
               os << "\\f";
               break;
            case '\n':        // \x0a
               os << "\\n";
               break;
            case '\r':        // \x0d
               os << "\\r";
               break;
            case '\t':        // \x09
               os << "\\t";
               break;
            case '\\':
               os << "\\\\";
               break;
            case '\"':
               os << "\\\"";
               break;
            case '\x00': os << "\\u0000"; break;
            case '\x01': os << "\\u0001"; break;
            case '\x02': os << "\\u0002"; break;
            case '\x03': os << "\\u0003"; break;
            case '\x04': os << "\\u0004"; break;
            case '\x05': os << "\\u0005"; break;
            case '\x06': os << "\\u0006"; break;
            case '\x07': os << "\\u0007"; break; // \a is not valid JSON
         // case '\x08': os << "\\u0008"; break; // \b
         // case '\x09': os << "\\u0009"; break; // \t
         // case '\x0a': os << "\\u000a"; break; // \n
            case '\x0b': os << "\\u000b"; break;
         // case '\x0c': os << "\\u000c"; break; // \f
         // case '\x0d': os << "\\u000d"; break; // \r
            case '\x0e': os << "\\u000e"; break;
            case '\x0f': os << "\\u000f"; break;

            case '\x10': os << "\\u0010"; break;
            case '\x11': os << "\\u0011"; break;
            case '\x12': os << "\\u0012"; break;
            case '\x13': os << "\\u0013"; break;
            case '\x14': os << "\\u0014"; break;
            case '\x15': os << "\\u0015"; break;
            case '\x16': os << "\\u0016"; break;
            case '\x17': os << "\\u0017"; break;
            case '\x18': os << "\\u0018"; break;
            case '\x19': os << "\\u0019"; break;
            case '\x1a': os << "\\u001a"; break;
            case '\x1b': os << "\\u001b"; break;
            case '\x1c': os << "\\u001c"; break;
            case '\x1d': os << "\\u001d"; break;
            case '\x1e': os << "\\u001e"; break;
            case '\x1f': os << "\\u001f"; break;

            default:
               os << *itr;
         }
      }
      os << '"';
   }
   ostream& json::to_stream( ostream& out, const std::string& str )
   {
        escape_string( str, out );
        return out;
   }

   template<typename T>
   void to_stream( T& os, const variants& a, json::output_formatting format, uint32_t max_depth )
   {
      os << '[';
      auto itr = a.begin();

      while( itr != a.end() )
      {
         to_stream( os, *itr, format, max_depth );
         ++itr;
         if( itr != a.end() )
            os << ',';
      }
      os << ']';
   }
   template<typename T>
   void to_stream( T& os, const variant_object& o, json::output_formatting format, uint32_t max_depth )
   {
       os << '{';
       auto itr = o.begin();

       while( itr != o.end() )
       {
          escape_string( itr->key(), os );
          os << ':';
          to_stream( os, itr->value(), format, max_depth );
          ++itr;
          if( itr != o.end() )
             os << ',';
       }
       os << '}';
   }

   template<typename T>
   void to_stream( T& os, const variant& v, json::output_formatting format, uint32_t max_depth )
   {
      FC_ASSERT( max_depth > 0, "Too many nested objects!" );
      switch( v.get_type() )
      {
         case variant::null_type:
              os << "null";
              return;
         case variant::int64_type:
              if( format == json::stringify_large_ints_and_doubles &&
                  ( v.as_int64() > INT32_MAX || v.as_int64() < INT32_MIN ) )
                 os << '"'<<v.as_string()<<'"';
              else
                 os << v.as_int64();
              return;
         case variant::uint64_type:
              if( format == json::stringify_large_ints_and_doubles &&
                  v.as_uint64() > 0xffffffff )
                 os << '"'<<v.as_string()<<'"';
              else
                 os << v.as_uint64();
              return;
         case variant::double_type:
              if (format == json::stringify_large_ints_and_doubles)
                 os << '"'<<v.as_string()<<'"';
              else
                 os << v.as_string();
              return;
         case variant::bool_type:
              os << v.as_string();
              return;
         case variant::string_type:
              escape_string( v.get_string(), os );
              return;
         case variant::blob_type:
              escape_string( v.as_string(), os );
              return;
         case variant::array_type:
              to_stream( os, v.get_array(), format, max_depth - 1 );
              return;
         case variant::object_type:
              to_stream(os, v.get_object(), format, max_depth - 1 );
              return;
         default:
            FC_THROW_EXCEPTION( fc::invalid_arg_exception, "Unsupported variant type: ${type}", ( "type", v.get_type() ) );
      }
   }

   std::string   json::to_string( const variant& v, output_formatting format, uint32_t max_depth )
   {
      fc::stringstream ss;
      fc::to_stream( ss, v, format, max_depth );
      return ss.str();
   }


    std::string pretty_print( const std::string& v, uint8_t indent ) {
      int level = 0;
      fc::stringstream ss;
      bool first = false;
      bool quote = false;
      bool escape = false;
      for( uint32_t i = 0; i < v.size(); ++i ) {
         switch( v[i] ) {
            case '\\':
              if( !escape ) {
                if( quote )
                  escape = true;
              } else { escape = false; }
              ss<<v[i];
              break;
            case ':':
              if( !quote ) {
                ss<<": ";
              } else {
                ss<<':';
              }
              break;
            case '"':
              if( first ) {
                 ss<<'\n';
                 for( int i = 0; i < level*indent; ++i ) ss<<' ';
                 first = false;
              }
              if( !escape ) {
                quote = !quote;
              }
              escape = false;
              ss<<'"';
              break;
            case '{':
            case '[':
              ss<<v[i];
              if( !quote ) {
                ++level;
                first = true;
              }else {
                escape = false;
              }
              break;
            case '}':
            case ']':
              if( !quote ) {
                if( v[i-1] != '[' && v[i-1] != '{' ) {
                  ss<<'\n';
                }
                --level;
                if( !first ) {
                  for( int i = 0; i < level*indent; ++i ) ss<<' ';
                }
                first = false;
                ss<<v[i];
                break;
              } else {
                escape = false;
                ss<<v[i];
              }
              break;
            case ',':
              if( !quote ) {
                ss<<',';
                first = true;
              } else {
                escape = false;
                ss<<',';
              }
              break;
            case 'n':
              //If we're in quotes and see a \n, just print it literally but unset the escape flag.
              if( quote && escape )
                escape = false;
              FALLTHROUGH
            default:
              if( first ) {
                 ss<<'\n';
                 for( int i = 0; i < level*indent; ++i ) ss<<' ';
                 first = false;
              }
              ss << v[i];
         }
      }
      return ss.str();
    }



   std::string json::to_pretty_string( const variant& v, output_formatting format, uint32_t max_depth )
   {
	   return pretty_print(to_string(v, format, max_depth), 2);
   }

   void json::save_to_file( const variant& v, const fc::path& fi, bool pretty, output_formatting format, uint32_t max_depth )
   {
      if( pretty )
      {
        auto str = json::to_pretty_string( v, format, max_depth );
        fc::ofstream o(fi);
        o.write( str.c_str(), str.size() );
      }
      else
      {
       fc::ofstream o(fi);
       fc::to_stream( o, v, format, max_depth );
      }
   }
   variant json::from_file( const fc::path& p, parse_type ptype, uint32_t max_depth )
   {
      fc::istream_ptr in( new fc::ifstream( p ) );
      fc::buffered_istream bin( in );
      return from_stream( bin, ptype, max_depth );
   }
   variant json::from_stream( buffered_istream& in, parse_type ptype, uint32_t max_depth )
   {
      switch( ptype )
      {
          case legacy_parser:
              return variant_from_stream<fc::buffered_istream, legacy_parser>( in, max_depth );
#ifdef WITH_EXOTIC_JSON_PARSERS
          case legacy_parser_with_string_doubles:
              return variant_from_stream<fc::buffered_istream, legacy_parser_with_string_doubles>( in, max_depth );
          case strict_parser:
              return json_relaxed::variant_from_stream<buffered_istream, true>( in, max_depth );
          case relaxed_parser:
              return json_relaxed::variant_from_stream<buffered_istream, false>( in, max_depth );
#endif
          case broken_nul_parser:
              return variant_from_stream<fc::buffered_istream, broken_nul_parser>( in, max_depth );
          default:
              FC_ASSERT( false, "Unknown JSON parser type {ptype}", ("ptype", ptype) );
      }
   }

   ostream& json::to_stream( ostream& out, const variant& v, output_formatting format, uint32_t max_depth )
   {
      fc::to_stream( out, v, format, max_depth );
      return out;
   }
   ostream& json::to_stream( ostream& out, const variants& v, output_formatting format, uint32_t max_depth )
   {
      fc::to_stream( out, v, format, max_depth );
      return out;
   }
   ostream& json::to_stream( ostream& out, const variant_object& v, output_formatting format, uint32_t max_depth )
   {
      fc::to_stream( out, v, format, max_depth );
      return out;
   }

   bool json::is_valid( const std::string& utf8_str, parse_type ptype, uint32_t max_depth )
   {
      if( utf8_str.size() == 0 ) return false;
      fc::istream_ptr in( new fc::stringstream( utf8_str ) );
      fc::buffered_istream bin( in );
      from_stream( bin, ptype, max_depth );
      try { bin.peek(); } catch ( const eof_exception& e ) { return true; }
      return false;
   }

} // fc
