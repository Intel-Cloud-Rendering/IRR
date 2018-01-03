#include "GovernServer.h"
#include "RenderLog.h"

using namespace irr;

GovernServer::GovernServer(boost::asio::io_service& io_service, short port)
    : AsioServer(io_service, port) {
}

void GovernServer::handle_accept() {
    irr_log_info("handle govern");
}

void GovernServer::handle_terminate() {
}
