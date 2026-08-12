#ifndef LQR_RESOLVE_H
#define LQR_RESOLVE_H
  
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
  
/* Hardware coordinates for the LQR block's bridge registers,
 * resolved from the loaded config databases. */
struct lqr_coords {
    unsigned int block_base;    // Block base/type -> hw_write_register arg 1
    unsigned int block_number;  // Instance index  -> arg 2
    unsigned int start;         // GAINS_START register
    unsigned int data;          // GAINS_DATA register
    unsigned int commit;        // COMMIT register
    unsigned int gen;           // GEN register
};  
      
/* Resolve the LQQR block + its GAINS/COMMIT/GEN fields.
 * On success, fills and returns true.
 * On failure, logs and returns false. */
bool lqr_resolve(struct lqr_coords *out);

#ifdef __cplusplus
}
#endif
 
#endif