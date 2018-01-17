#include "RenderServer.h"
#include "RenderLog.h"
#include "android/cmdline-option.h"
#include <signal.h>

using namespace irr;
using namespace std;

shared_ptr<RenderServer> rs = NULL;

void sigint_handler(int sig_num) {
  irr_log_info("ctrl-c detected...");
  if (rs) {
    rs->terminate();
  }
}

int main(int argc, char* argv[])
{
  signal(SIGINT, sigint_handler);
  try
  {
    AndroidOptions opt;
    android_cmdLineOptions = &opt;
    if ((argc < 2) || (android_parse_options(argc, argv, &opt) != 0))
    {
      irr_log_err("Usage: async_tcp_echo_server <port> [-<option> <option_value>]\n");
      return 1;
    }

    boost::asio::io_service io_service;

    rs = make_shared<RenderServer>(io_service, std::atoi(argv[1]));
    rs->init();
    rs->run();
    /* handler executes only in main thread *
     * so use explicit strand ensures thread safity */
    io_service.run();
  }
  catch (std::exception& e)
  {
    irr_log_err("Exception: ", e.what());
  }

  return 0;
}
