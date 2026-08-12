// Implemented in C++ (lqr_bridge.cpp),
// called from the C server.
#ifndef LQR_BRIDGE_H
#define LQR_BRIDGE_H

// Macro not defined in C => C++ only
#ifdef __cplusplus
extern "C" {
#endif

// Spawn the bridge thread.
void lqr_bridge_start(void);
// Stop and join the bridge thread.
void lqr_bridge_stop(void);

#ifdef __cplusplus
}
#endif

#endif
