// Implemented in C++ (lqr_bridge.cpp),
// called from the C server.
#ifndef LQR_BRIDGE_H
#define LQR_BRIDGE_H

#include <stdint.h>

// Macro not defined in C => C++ only
#ifdef __cplusplus
extern "C" {
#endif

// Spawn the bridge thread.
void lqr_bridge_start(void);
// Stop and join the bridge thread.
void lqr_bridge_stop(void);

// Health seam for a status PV / supervisor: the startup state-window
// read-latency ratio (vs a cacheable heap word) * 1000.
//  - -1 = not yet probed
//  - ~1000 = cacheable (good)
//  - >~4000 = window looks non-cacheable
// Safe to poll from any thread.
int32_t lqr_bridge_cache_ratio_milli(void);

#ifdef __cplusplus
}
#endif

#endif
