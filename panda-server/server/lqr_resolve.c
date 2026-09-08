#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "error.h"
#include "config_server.h"
#include "hardware.h"
#include "parse.h"
#include "hashtable.h"
#include "attributes.h"
#include "fields.h"
#include "lqr_resolve.h"
    
/* Device-tree property cells are big-endian regardless of the CPU. */
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) |
           ((uint32_t) p[2] <<  8) |  (uint32_t) p[3];
}

static error__t resolve_state_buffer(uint64_t *phys, uint64_t *bytes)
{
    const char *base_dir = "/proc/device-tree/reserved-memory";

    DIR *dir = opendir(base_dir);
    if (dir == NULL) {
        return FAIL_("LQR bridge: no reserved-memory in the device tree");
    }

    char reg_path[512];
    bool found = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "lqr-state", 9) == 0) {
            snprintf(reg_path, sizeof(reg_path),
                "%s/%s/reg", base_dir, ent->d_name);
            found = true;
            break;
        }
    }
    closedir(dir);

    if (!found) {
        return FAIL_("LQR bridge: reserved-memory node 'lqr-state' not found");
    }

    int fd = open(reg_path, O_RDONLY);
    if (fd < 0) {
        return FAIL_("LQR bridge: cannot open the state reg property");
    }

    uint8_t reg[8];
    ssize_t n = read(fd, reg, sizeof(reg));
    close(fd);

    if (n != (ssize_t) sizeof(reg)) {
        return FAIL_("LQR bridge: unexpected reserved-memory reg size");
    }

    *phys  = be32(reg);      // base cell
    *bytes = be32(reg + 4);  // size cell
    return ERROR_OK;
}

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
        set_field_read_only(gains)  ?: // bridge owns the gain stream
        set_field_read_only(commit);   // and the swap trigger
        
    if (ERROR_REPORT(error, "LQR bridge: could not resolve LQR block")) {
        return false;
    }

    uint64_t state_phys = 0, state_bytes = 0;
    if (ERROR_REPORT(
            resolve_state_buffer(&state_phys, &state_bytes),
            "LQR bridge: could not resolve the state buffer"
        )
    ) {
        return false;
    }

    *out = (struct lqr_coords) {
        .block_base   = get_block_base(block),
        .block_number = 0, // single-instance block
        .start        = gains_regs[0], // GAINS_START (init_reg)
        .data         = gains_regs[1], // GAINS_DATA  (fill_reg)
        .commit       = commit_regs[0],
        .gen          = gen_regs[0],
        .state_phys   = state_phys,
        .state_bytes  = state_bytes,
    };
    return true;
}