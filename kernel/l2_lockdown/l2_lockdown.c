// SPDX-License-Identifier: GPL-2.0
/*
 * Confines the ACP master to a single L2 way + locks CPU masters out of
 * that way.
 *
 * Load *after* l2x0 init and *before* the bridge starts ACP traffic.
 * Unloading restores unrestricted allocation for every master touched.
 * Pre-existing lines are not relocated by the mask write.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/kernel.h>

#define PL310_DEFAULT_BASE 0xF8F02000u // Zynq-7000 PL310 register block
#define PL310_REGION_SIZE 0x1000u

/* Lockdown-by-master: 8 (Data,Instr) register pairs, 8 bytes apart. */
#define L2X0_LOCKDOWN_D(n) (0x900u + 8u * (n))
#define L2X0_LOCKDOWN_I(n) (0x904u + 8u * (n))
#define L2X0_NUM_MASTERS 8
#define L2X0_WAY_MASK 0xFFu // 8-way PL310 on the -7030

static unsigned int pl310_base = PL310_DEFAULT_BASE;
module_param(pl310_base, uint, 0444);
MODULE_PARM_DESC(pl310_base, "PL310 register block physical base");

/*
 * ACP -> lockdown index 1, CPUs -> index 0.
 *
 * Derivation (L2C-310 TRM, 1x A9 MPCore + ACP, 8-way):
 * per-master ID seen by the lockdown logic is AxUSER[7:5],
 * driven on the A9 MPCore as {2'b00, AxIDM[2]},
 * where IDM[2] == 1 iff the transaction is ACP.
 *
 * So USER[7:5] == 0b001 == 1 for ACP and 0b000 == 0 for the CPUs.
 * Both cores share index 0, only 0 and 1 are ever driven =>
 * lockdown registers 2..7 are dead.
 */
static int acp_index = 1;
module_param(acp_index, int, 0444);
MODULE_PARM_DESC(acp_index, "Lockdown index of the ACP master (Zynq: 1)");

static int cpu_indices[L2X0_NUM_MASTERS] = { 0 };
static int cpu_count = 1;
module_param_array(cpu_indices, int, &cpu_count, 0444);
MODULE_PARM_DESC(cpu_indices, "Lockdown indices of the CPU masters (Zynq: 0)");

static unsigned int acp_mask = 0x7Fu; // lock ways 0-6 -> ACP allocates only in way 7
module_param(acp_mask, uint, 0444);
MODULE_PARM_DESC(acp_mask, "Way-lock bitmask for ACP");

static unsigned int cpu_mask = 0x80u; // lock way 7 -> CPUs allocate in ways 0-6
module_param(cpu_mask, uint, 0444);
MODULE_PARM_DESC(cpu_mask, "Way-lock bitmask for the CPU masters");

static void __iomem *pl310;

static void set_master(int idx, unsigned int mask)
{
	// Device-nGnRE mapping keeps these ordered per-peripheral.
	writel_relaxed(mask, pl310 + L2X0_LOCKDOWN_D(idx));
	writel_relaxed(mask, pl310 + L2X0_LOCKDOWN_I(idx));
}

static int __init l2_lockdown_init(void)
{
	unsigned int allowed_acp, allowed_cpu;
	int i;

	// Warn if a bad config
	if (acp_index < 0 || acp_index >= L2X0_NUM_MASTERS) {
		pr_err("l2_lockdown: acp_index=%d out of range (0-%d)\n",
		       acp_index, L2X0_NUM_MASTERS - 1);
		return -EINVAL;
	}
	if ((acp_mask & ~L2X0_WAY_MASK) || (cpu_mask & ~L2X0_WAY_MASK)) {
		pr_err("l2_lockdown: mask exceeds the %d ways present\n", L2X0_NUM_MASTERS);
		return -EINVAL;
	}
	for (i = 0; i < cpu_count; i++) {
		if (cpu_indices[i] < 0 || cpu_indices[i] >= L2X0_NUM_MASTERS) {
			pr_err("l2_lockdown: cpu_indices[%d]=%d out of range\n", i, cpu_indices[i]);
			return -EINVAL;
		}
	}

	// The whole point is a disjoint partition - warn loudly if it isn't!
	allowed_acp = (~acp_mask) & L2X0_WAY_MASK;
	allowed_cpu = (~cpu_mask) & L2X0_WAY_MASK;
	if (allowed_acp & allowed_cpu) {
		pr_warn("l2_lockdown: ACP and CPU share way(s) %#04x - ACP can still evict the RT set\n",
			allowed_acp & allowed_cpu);
	}
	if (!allowed_acp) {
		pr_warn("l2_lockdown: ACP has no allocatable way (mask %#04x)\n", acp_mask);
	}

	pl310 = ioremap(pl310_base, PL310_REGION_SIZE);
	if (!pl310) {
		pr_err("l2_lockdown: ioremap(%#x) failed\n", pl310_base);
		return -ENOMEM;
	}

	// Lockdown CPU masters first, then confine ACP.
	// This keeps the CPU masters out of the way of the ACP.
	for (i = 0; i < cpu_count; i++) {
		set_master(cpu_indices[i], cpu_mask);
	}
	// Now lockdown ACP
	set_master(acp_index, acp_mask);

	// Verify the lockdown succeeded
	pr_info("l2_lockdown: ACP idx %d -> D=%#04x I=%#04x; CPU mask %#04x on %d master(s)\n",
		acp_index,
		readl_relaxed(pl310 + L2X0_LOCKDOWN_D(acp_index)),
		readl_relaxed(pl310 + L2X0_LOCKDOWN_I(acp_index)),
		cpu_mask, cpu_count);

	return 0;
}

static void __exit l2_lockdown_exit(void)
{
	int i;

	// Restore unrestricted allocation for every master touched.
	set_master(acp_index, 0);
	for (i = 0; i < cpu_count; i++) {
		set_master(cpu_indices[i], 0);
	}
	(void)readl_relaxed(pl310 + L2X0_LOCKDOWN_D(acp_index)); // flush
	iounmap(pl310);
	pr_info("l2_lockdown: lockdown cleared\n");
}

module_init(l2_lockdown_init);
module_exit(l2_lockdown_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diamond Light Source");
MODULE_DESCRIPTION("PL310 lockdown-by-master: confine ACP to one L2 way for RT determinism");
