// SPDX-License-Identifier: GPL-2.0-only
/*
 * azhub: lets the XDJ-AZ enumerate a USB hub plugged into USB1 or USB2.
 *
 * The deck's kernel is built with CONFIG_ATC, which sets MAX_TOPO_LEVEL to 1
 * (include/linux/usb/hcd.h in the vendor tree), so hub_probe() refuses every
 * external hub with "Unsupported bus topology: hub nested too deep" and raises
 * HUBERR, the "Hub not supported" caution. The one comparison in hub_probe is
 *
 *	if (hdev->level == MAX_TOPO_LEVEL)	ldrb w0, [x24,#1275]; cmp w0, #0x1; b.ne
 *
 * This module rewrites that cmp to compare with 6, the stock Linux limit, and
 * puts the original instruction back when it is removed.
 *
 * Exact identity, inert otherwise: the patch is written only when the four
 * instructions around it match the 4.4.194 build of firmware 2.00 word for word.
 * Anything else (another firmware, an already patched kernel) refuses the load
 * and changes nothing.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>

#define AZHUB_CMP_OFF	0x84

/* hub_probe+0x7c .. +0x88 on firmware 2.00 */
static const u32 azhub_expect[] = {
	0x3953ef00,	/* ldrb w0, [x24,#1275]   hdev->level */
	0x9100c354,	/* add  x20, x26, #0x30 */
	0x7100041f,	/* cmp  w0, #0x1          MAX_TOPO_LEVEL (CONFIG_ATC) */
	0x540002c1,	/* b.ne hub_probe+0xe0 */
};
#define AZHUB_CMP_STOCK	0x7100041f	/* cmp w0, #0x1 */
#define AZHUB_CMP_MOD	0x7100181f	/* cmp w0, #0x6 */

typedef int (*azhub_patch_fn)(void *addrs[], u32 insns[], int cnt);

static azhub_patch_fn azhub_patch;
static u32 *azhub_cmp;

static int azhub_write(u32 insn)
{
	void *addrs[1] = { azhub_cmp };
	u32 insns[1] = { insn };

	return azhub_patch(addrs, insns, 1);
}

static int __init azhub_init(void)
{
	unsigned long probe = kallsyms_lookup_name("hub_probe");
	const u32 *p;
	int i, ret;

	azhub_patch = (azhub_patch_fn)kallsyms_lookup_name("aarch64_insn_patch_text");
	if (!probe || !azhub_patch) {
		pr_err("azhub: hub_probe or aarch64_insn_patch_text not found\n");
		return -ENODEV;
	}
	p = (const u32 *)(probe + AZHUB_CMP_OFF - 8);
	for (i = 0; i < ARRAY_SIZE(azhub_expect); i++) {
		if (p[i] != azhub_expect[i]) {
			pr_err("azhub: unknown kernel build (hub_probe+%#x = %08x), not patching\n",
			       AZHUB_CMP_OFF - 8 + i * 4, p[i]);
			return -ENODEV;
		}
	}
	azhub_cmp = (u32 *)(probe + AZHUB_CMP_OFF);
	ret = azhub_write(AZHUB_CMP_MOD);
	if (ret) {
		pr_err("azhub: patch failed (%d)\n", ret);
		return ret;
	}
	pr_info("azhub: external USB hubs enabled (topology limit 1 -> 6)\n");
	return 0;
}

static void __exit azhub_exit(void)
{
	if (azhub_write(AZHUB_CMP_STOCK))
		pr_err("azhub: restore failed\n");
	else
		pr_info("azhub: topology limit restored to 1\n");
}

module_init(azhub_init);
module_exit(azhub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AZ-MODS: allow USB hubs on the XDJ-AZ");
