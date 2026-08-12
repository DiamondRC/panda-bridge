#include "lqr_bridge.h"

// We need only log_message from the server.
// error.h not C++ clean => declare the only thing we need
extern "C" void log_message(const char *message, ...);

extern "C" void lqr_bridge_start(void)
{
    log_message("LQR bridge: start (skeleton)");
}

extern "C" void lqr_bridge_stop(void)
{
    log_message("LQR bridge: stop (skeleton)");
}