#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(bq27xxx_battery_update, "_gpl", "");
KSYMTAB_FUNC(bq27xxx_battery_setup, "_gpl", "");
KSYMTAB_FUNC(bq27xxx_battery_teardown, "_gpl", "");
KSYMTAB_DATA(bq27xxx_battery_battery_pm_ops, "_gpl", "");

SYMBOL_CRC(bq27xxx_battery_update, 0x13150311, "_gpl");
SYMBOL_CRC(bq27xxx_battery_setup, 0xfa558af4, "_gpl");
SYMBOL_CRC(bq27xxx_battery_teardown, 0x14b9f417, "_gpl");
SYMBOL_CRC(bq27xxx_battery_battery_pm_ops, 0x9691726f, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xf75d607d, "power_supply_get_drvdata" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xea4a09cb, "mod_delayed_work_on" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x29d9f26e, "cancel_delayed_work_sync" },
	{ 0x58b4691, "param_set_uint" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x7256d921, "_dev_err" },
	{ 0x88fdb6d8, "power_supply_changed" },
	{ 0x71c90087, "memcmp" },
	{ 0x526c3a6c, "jiffies" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xc34fa6f0, "devm_power_supply_register_no_ws" },
	{ 0xb7a68292, "power_supply_get_battery_info" },
	{ 0x5f754e5a, "memset" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0x46c4a691, "param_get_uint" },
	{ 0xac564f31, "param_ops_bool" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "6C8F28793972A53DE588A1E");
