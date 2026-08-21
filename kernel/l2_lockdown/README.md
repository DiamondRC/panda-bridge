# l2_lockdown - PL310 way partitioning for the RT core

Confines the **ACP** master to a single L2 way and keeps the **CPU** masters
out of it, so the PL-pushed state snapshot (ACP writes into L2) can never
evict the RT optimiser's working set. Locked ways still *hit* on read; only
allocation is restricted - the ACP-pollution jitter risk is bounded
**structurally**, not measured-and-hoped. (Deployment plan, Phase 5.)

## Mask semantics

PL310 way-lock bitmask: **bit N set ⇒ that master may not *allocate* in way N.**

| Master | Param default | Effect (8-way part) |
|--------|---------------|---------------------|
| ACP    | `acp_mask=0x7F` | ways 0–6 locked ⇒ allocates only in **way 7** |
| CPUs   | `cpu_mask=0x80` | way 7 locked ⇒ allocate in **ways 0–6** |

Allowed ways are disjoint by construction; the module warns if a chosen mask
pair overlaps (ACP could then still evict the RT set).

## Master -> lockdown index (confirmed)

From the L2C-310 TRM (1x A9 MPCore + ACP, 8-way): the lockdown logic sees the
master ID on `AxUSER[7:5]`, driven as `{2'b00, AxIDM[2]}`, and `IDM[2] == 1`
iff the transaction is ACP. So `USER[7:5]` = `1` for **ACP -> index 1**
(`D1=0x908, I1=0x90C`) and `0` for **both CPU cores -> index 0**
(`D0=0x900, I0=0x904`) - the cores are not distinguished, and only indices 0
and 1 are ever driven (2-7 are dead). These are the module defaults.

**Still verify on-silicon** via the L2 PMU: with ACP traffic running, its
allocations must land only in way 7 and the CPU set's eviction rate must not
move. The datasheet gives the mapping; the PMU proves the silicon honours it.

## Build

Against the same kernel the rootfs produced (cross):

```sh
make KDIR=../../rootfs/build/linux \
     ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## Load (ordering matters)

After `l2x0` init (any post-boot point) and **before** the bridge starts ACP
traffic - so the partition is in place before the RT set is warmed:

```sh
insmod l2_lockdown.ko    # defaults are the confirmed Zynq values
# explicit equivalent: acp_index=1 cpu_indices=0 acp_mask=0x7F cpu_mask=0x80
```

Verify via dmesg (the readback line) and the L2 PMU
(`CONFIG_CACHE_L2X0_PMU=y`): ACP allocations must land only in its way, and
the RT set's eviction rate must not move when ACP traffic starts.

`rmmod l2_lockdown` restores unrestricted allocation.
