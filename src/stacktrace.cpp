//
// A stacktrace handler for bitshares
//

#include <signal.h>
#include <fc/log/logger.hpp>
#include <boost/stacktrace.hpp>

namespace fc
{

static void segfault_signal_handler(int signum)
{
   ::signal(signum, SIG_DFL);
   std::stringstream ss;
   ss << boost::stacktrace::stacktrace();
   elog(ss.str());
   ::raise(SIGABRT);
}

void print_stacktrace_on_segfault()
{
   ::signal(SIGSEGV, &segfault_signal_handler);
}

void print_stacktrace(std::ostream& out)
{
   out << boost::stacktrace::stacktrace();
}

}
