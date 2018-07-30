#include <fc/rpc/cli.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef HAVE_EDITLINE
# include "editline.h"
# ifdef WIN32
#  include <io.h>
# endif
#endif

namespace fc { namespace rpc {

static std::vector<std::string>& cli_commands()
{
   static std::vector<std::string>* cmds = new std::vector<std::string>();
   return *cmds;
}

cli::~cli()
{
   if( _run_complete.valid() )
   {
      stop();
   }
}

variant cli::send_call( api_id_type api_id, string method_name, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

variant cli::send_callback( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void cli::send_notice( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void cli::start()
{
   cli_commands() = get_method_names(0);
   _run_complete = fc::async( [&](){ run(); } );
}

void cli::stop()
{
   _run_complete.cancel();
   _run_complete.wait();
}

void cli::wait()
{
   _run_complete.wait();
}

void cli::format_result( const string& method, std::function<string(variant,const variants&)> formatter)
{
   _result_formatters[method] = formatter;
}

void cli::set_prompt( const string& prompt )
{
   _prompt = prompt;
}

void cli::run()
{
   while( !_run_complete.canceled() )
   {
      try
      {
         std::string line;
         try
         {
            getline( _prompt.c_str(), line );
         }
         catch ( const fc::eof_exception& e )
         {
            break;
         }
         std::cout << line << "\n";
         line += char(EOF);
         fc::variants args = fc::json::variants_from_string(line);;
         if( args.size() == 0 )
            continue;

         const string& method = args[0].get_string();

         auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
         auto itr = _result_formatters.find( method );
         if( itr == _result_formatters.end() )
         {
            std::cout << fc::json::to_pretty_string( result ) << "\n";
         }
         else
            std::cout << itr->second( result, args ) << "\n";
      }
      catch ( const fc::exception& e )
      {
         std::cout << e.to_detail_string() << "\n";

         if (e.code() == fc::canceled_exception_code)
         {
            break;
         }
      }
   }
}

/****
 * @brief loop through list of commands, attempting to find a match
 * @param token what the user typed
 * @param match sets to 1 if only 1 match was found
 * @returns the remaining letters of the name of the command or NULL if 1 match not found
 */
static char *my_rl_complete(char *token, int *match)
{
   bool have_one = false;
   std::string method_name;

   auto& cmd = cli_commands();
   const size_t partlen = strlen (token); /* Part of token */

   for (const std::string& it : cmd)
   {
      if (it.compare(0, partlen, token) == 0)
      {
         if (have_one) {
            // we can only have 1, but we found a second
            return NULL;
         }
         else
         {
            method_name = it;
            have_one = true;
         }
      }
   }

   if (have_one)
   {
      *match = 1;
      method_name += " ";
      return strdup (method_name.c_str() + partlen);
   }

   return NULL;
}

/***
 * @brief return an array of matching commands
 * @param token the incoming text
 * @param array the resultant array of possible matches
 * @returns the number of matches
 */
static int cli_completion(char *token, char ***array)
{
   auto& cmd = cli_commands();
   int num_commands = cmd.size();

   char **copy = (char **) malloc (num_commands * sizeof(char *));
   if (copy == NULL)
   {
      // possible out of memory
      return 0;
   }
   int total_matches = 0;

   const size_t partlen = strlen(token);

   for (const std::string& it : cmd)
   {
      if ( it.compare(0, partlen, token) == 0)
      {
         copy[total_matches] = strdup ( it.c_str() );
         ++total_matches;
      }
   }
   *array = copy;

   return total_matches;
}

/***
 * @brief Read input from the user
 * @param prompt the prompt to display
 * @param line what the user typed
 */
void cli::getline( const fc::string& prompt, fc::string& line)
{
   // getting file descriptor for C++ streams is near impossible
   // so we just assume it's the same as the C stream...
#ifdef HAVE_EDITLINE
#ifndef WIN32   
   if( isatty( fileno( stdin ) ) )
#else
   // it's implied by
   // https://msdn.microsoft.com/en-us/library/f4s0ddew.aspx
   // that this is the proper way to do this on Windows, but I have
   // no access to a Windows compiler and thus,
   // no idea if this actually works
   if( _isatty( _fileno( stdin ) ) )
#endif
   {
      rl_set_complete_func(my_rl_complete);
      rl_set_list_possib_func(cli_completion);

      static fc::thread getline_thread("getline");
      getline_thread.async( [&](){
         char* line_read = nullptr;
         std::cout.flush(); //readline doesn't use cin, so we must manually flush _out
         line_read = readline(prompt.c_str());
         if( line_read == nullptr )
            FC_THROW_EXCEPTION( fc::eof_exception, "" );
         line = line_read;
         try
         {
            if (*line_read)
               add_history(line_read);
         }
         catch(...)
         {
            free(line_read);
            throw;
         }
         free(line_read);
      }).wait();
   }
   else
#endif
   {
      std::cout << prompt;
      // sync_call( cin_thread, [&](){ std::getline( *input_stream, line ); }, "getline");
      fc::getline( fc::cin, line );
      return;
   }
}

} } // namespace fc::rpc
