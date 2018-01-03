#ifndef GOVERN_SERVER
#define GOVERN_SERVER
#include "AsioServer.h"

namespace irr {

  class GovernServer : public AsioServer {
 public:
    GovernServer(boost::asio::io_service& io_service, short port);
 private:
    virtual void handle_accept();
    virtual void handle_terminate();
  };

}

#endif
