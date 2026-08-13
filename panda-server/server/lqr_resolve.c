#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include "error.h"
#include "config_server.h"
#include "hardware.h"
#include "parse.h"
#include "hashtable.h"
#include "attributes.h"
#include "fields.h"
#include "lqr_resolve.h"
    
bool lqr_resolve(struct lqr_coords *out)
{   
    struct block *block;
    unsigned int block_count;
    struct field *gains, *commit, *gen;
    unsigned int gains_regs[3], commit_regs[3], gen_regs[3];
    size_t reg_count;
        
    error__t error =
        lookup_block("LQR", &block, &block_count)  ?:
        lookup_field(block, "GAINS",  &gains) ?:
        lookup_field(block, "COMMIT", &commit) ?:
        lookup_field(block, "GEN",    &gen) ?:
        get_field_registers(gains,  gains_regs,  &reg_count) ?:
        get_field_registers(commit, commit_regs, &reg_count) ?:
        get_field_registers(gen,    gen_regs,    &reg_count) ?:
        get_field_registers(gen, gen_regs, &reg_count)  ?:
        set_field_read_only(gains)  ?: // bridge owns the gain stream
        set_field_read_only(commit);   // and the swap trigger
        
    if (ERROR_REPORT(error, "LQR bridge: could not resolve LQR block"))
        return false; 
        
    *out = (struct lqr_coords) {
        .block_base   = get_block_base(block),
        .block_number = 0, // single-instance block
        .start        = gains_regs[0], // GAINS_START (init_reg)
        .data         = gains_regs[1], // GAINS_DATA  (fill_reg)
        .commit       = commit_regs[0],
        .gen          = gen_regs[0],
    }; 
    return true;
}