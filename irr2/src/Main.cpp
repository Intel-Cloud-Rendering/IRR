#include "RenderServer.h"
#include "GovernServer.h"
#include "RenderLog.h"
#include <signal.h>

using namespace irr;
using namespace std;

shared_ptr<RenderServer> rs = NULL;
shared_ptr<GovernServer> gs = NULL;

void sigint_handler(int sig_num) {
  irr_log_info("ctrl-c detected...");
  if (rs) {
    rs->terminate();
  }
  if (gs) {
    gs->terminate();
  }
}

int main(int argc, char* argv[])
{
  bool use_govern_server = false;
  signal(SIGINT, sigint_handler);
  try
  {
    if (argc != 2)
    {
      irr_log_err("Usage: async_tcp_echo_server <port>\n");
      return 1;
    }

    boost::asio::io_service io_service;

    rs = make_shared<RenderServer>(io_service, std::atoi(argv[1]));
    rs->init();
    rs->run();
    if (use_govern_server) {
      gs = make_shared<GovernServer>(io_service, std::atoi(argv[1]) + 1);
      gs->run();
    }
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
