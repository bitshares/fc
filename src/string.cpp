#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/io/sstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <locale>
#include <limits>

/**
 *  Implemented with std::string for now.
 */

namespace fc  {
   class comma_numpunct : public std::numpunct<char>
   {
      protected:
         virtual char do_thousands_sep() const { return ','; }
         virtual std::string do_grouping() const { return "\03"; }
   };

  std::string to_pretty_string( int64_t value )
  {
     std::stringstream ss;
     ss.imbue( {std::locale(), new comma_numpunct} );
     ss << std::fixed << value;
     return ss.str();
  }

  int64_t    to_int64( const std::string& i )
  {
    try
    {
      return boost::lexical_cast<int64_t>(i.c_str());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse int64_t" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => int64_t", ("i",i) )
  }

  uint64_t   to_uint64( const std::string& i )
  { try {
    try
    {
      return boost::lexical_cast<uint64_t>(i.c_str());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse uint64_t" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => uint64_t", ("i",i) )
  } FC_CAPTURE_AND_RETHROW( (i) ) }

  double     to_double( const std::string& i)
  {
    try
    {
      return boost::lexical_cast<double>(i.c_str());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse double" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => double", ("i",i) )
  }

  std::string to_string(double d)
  {
    // +2 is required to ensure that the double is rounded correctly when read back in.  http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 2) << std::fixed << d;
    return ss.str();
  }

  std::string to_string( uint64_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }

  std::string to_string( int64_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }
  std::string to_string( uint16_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }
  std::string trim( const std::string& s )
  {
      return boost::algorithm::trim_copy(s);
  }
  std::string to_lower( const std::string& s )
  {
     auto tmp = s;
     boost::algorithm::to_lower(tmp);
     return tmp;
  }
  string trim_and_normalize_spaces( const string& s )
  {
     string result = boost::algorithm::trim_copy( s );
     while( result.find( "  " ) != result.npos )
       boost::algorithm::replace_all( result, "  ", " " );
     return result;
  }

  /**
   * Parses a size including an optional multiplicative suffix.
   *
   * M   -> 1024*1024 bytes
   * MB  -> 1000*1000 bytes
   * MiB -> 1024*1024 bytes
   *
   * The 'M' may be any of KMGTPEZY (upper or lower case)
   */
  uint64_t parse_size( const string& s )
  {
     try
     {
        size_t i = 0, n = s.size(), suffix_start = n;
        for( i=0; i<n; i++ )
        {
           if( !((s[i] >= '0') && (s[i] <= '9')) )
           {
              suffix_start = i;
              break;
           }
        }
        uint64_t u = to_uint64( s.substr( 0, suffix_start ) );

        FC_ASSERT( n - suffix_start <= 3 );

        uint64_t m = 1;
        uint64_t thousand = 1024;

        if( suffix_start == n )
        {
           return u;
        }
        else if( suffix_start == n-1 )
        {
        }
        else if( suffix_start == n-2 )
        {
           FC_ASSERT( (s[suffix_start+1] == 'b') || (s[suffix_start+1] == 'B') );
           thousand = 1000;
        }
        else if( suffix_start == n-3 )
        {
           FC_ASSERT( (s[suffix_start+1] == 'i') || (s[suffix_start+1] == 'I') );
           FC_ASSERT( (s[suffix_start+2] == 'b') || (s[suffix_start+2] == 'B') );
        }
        switch( s[suffix_start] )
        {
           case 'y':
           case 'Y':
              m *= thousand;
           case 'z':
           case 'Z':
              m *= thousand;
           case 'e':
           case 'E':
              m *= thousand;
           case 'p':
           case 'P':
              m *= thousand;
           case 't':
           case 'T':
              m *= thousand;
           case 'g':
           case 'G':
              m *= thousand;
           case 'm':
           case 'M':
              m *= thousand;
           case 'k':
           case 'K':
              m *= thousand;
              break;
           default:
              FC_ASSERT( false );
        }
        return u*m;
     }
     catch( const fc::exception& e )
     {
        FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse size" );
     }
  }

   string format_string( const string& format, const variant_object& args, uint32_t max_object_depth )
   {
      stringstream ss;
      size_t prev = 0;
      auto next = format.find( '$' );
      while( prev < format.size() )
      {
         ss << format.substr( prev, next == string::npos ? string::npos : next - prev );

         // if we got to the end, return it.
         if( next == size_t(string::npos) || next == format.size() )
            return ss.str();

         // if we are not at the end, then update the start
         prev = next + 1;

         if( format[prev] == '{' )
         {
            // if the next char is a open, then find close
            next = format.find( '}', prev );
            // if we found close...
            if( next != string::npos )
            {
               // the key is between prev and next
               string key = format.substr( prev+1, (next-prev-1) );

               auto val = args.find( key );
               if( val != args.end() )
               {
                  if( val->value().is_object() || val->value().is_array() )
                  {
                     try
                     {
                        ss << json::to_string( val->value(), json::stringify_large_ints_and_doubles, max_object_depth );
                     }
                     catch( const fc::assert_exception& e )
                     {
                        ss << "[\"ERROR_WHILE_CONVERTING_VALUE_TO_STRING\"]";
                     }
                  }
                  else
                     ss << val->value().as_string();
               }
               else
                  ss << "${"<<key<<"}";
               prev = next + 1;
            }
         }
         else
            ss << format[next];
         next = format.find( '$', prev );
      }
      return ss.str();
   }

} // namespace fc
