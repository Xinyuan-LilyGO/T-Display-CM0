#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x8c16b828, "i2c_register_driver" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x14b9f417, "bq27xxx_battery_teardown" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x13150311, "bq27xxx_battery_update" },
	{ 0x9d669763, "memcpy" },
	{ 0xbed0b84d, "i2c_transfer" },
	{ 0x1901e0f0, "i2c_smbus_read_i2c_block_data" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xa24491bf, "ida_free" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x4aed3ef3, "i2c_client_get_device_id" },
	{ 0xa5684076, "ida_alloc_range" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0x37ae6eab, "devm_kasprintf" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xfa558af4, "bq27xxx_battery_setup" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xcf86cdac, "queue_delayed_work_on" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x7256d921, "_dev_err" },
	{ 0x9691726f, "bq27xxx_battery_battery_pm_ops" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "bq27xxx_battery");

MODULE_ALIAS("of:N*T*Cti,bq27200");
MODULE_ALIAS("of:N*T*Cti,bq27200C*");
MODULE_ALIAS("of:N*T*Cti,bq27210");
MODULE_ALIAS("of:N*T*Cti,bq27210C*");
MODULE_ALIAS("of:N*T*Cti,bq27500");
MODULE_ALIAS("of:N*T*Cti,bq27500C*");
MODULE_ALIAS("of:N*T*Cti,bq27510");
MODULE_ALIAS("of:N*T*Cti,bq27510C*");
MODULE_ALIAS("of:N*T*Cti,bq27520");
MODULE_ALIAS("of:N*T*Cti,bq27520C*");
MODULE_ALIAS("of:N*T*Cti,bq27500-1");
MODULE_ALIAS("of:N*T*Cti,bq27500-1C*");
MODULE_ALIAS("of:N*T*Cti,bq27510g1");
MODULE_ALIAS("of:N*T*Cti,bq27510g1C*");
MODULE_ALIAS("of:N*T*Cti,bq27510g2");
MODULE_ALIAS("of:N*T*Cti,bq27510g2C*");
MODULE_ALIAS("of:N*T*Cti,bq27510g3");
MODULE_ALIAS("of:N*T*Cti,bq27510g3C*");
MODULE_ALIAS("of:N*T*Cti,bq27520g1");
MODULE_ALIAS("of:N*T*Cti,bq27520g1C*");
MODULE_ALIAS("of:N*T*Cti,bq27520g2");
MODULE_ALIAS("of:N*T*Cti,bq27520g2C*");
MODULE_ALIAS("of:N*T*Cti,bq27520g3");
MODULE_ALIAS("of:N*T*Cti,bq27520g3C*");
MODULE_ALIAS("of:N*T*Cti,bq27520g4");
MODULE_ALIAS("of:N*T*Cti,bq27520g4C*");
MODULE_ALIAS("of:N*T*Cti,bq27521");
MODULE_ALIAS("of:N*T*Cti,bq27521C*");
MODULE_ALIAS("of:N*T*Cti,bq27530");
MODULE_ALIAS("of:N*T*Cti,bq27530C*");
MODULE_ALIAS("of:N*T*Cti,bq27531");
MODULE_ALIAS("of:N*T*Cti,bq27531C*");
MODULE_ALIAS("of:N*T*Cti,bq27220");
MODULE_ALIAS("of:N*T*Cti,bq27220C*");
MODULE_ALIAS("of:N*T*Cti,bq27541");
MODULE_ALIAS("of:N*T*Cti,bq27541C*");
MODULE_ALIAS("of:N*T*Cti,bq27542");
MODULE_ALIAS("of:N*T*Cti,bq27542C*");
MODULE_ALIAS("of:N*T*Cti,bq27546");
MODULE_ALIAS("of:N*T*Cti,bq27546C*");
MODULE_ALIAS("of:N*T*Cti,bq27742");
MODULE_ALIAS("of:N*T*Cti,bq27742C*");
MODULE_ALIAS("of:N*T*Cti,bq27545");
MODULE_ALIAS("of:N*T*Cti,bq27545C*");
MODULE_ALIAS("of:N*T*Cti,bq27411");
MODULE_ALIAS("of:N*T*Cti,bq27411C*");
MODULE_ALIAS("of:N*T*Cti,bq27421");
MODULE_ALIAS("of:N*T*Cti,bq27421C*");
MODULE_ALIAS("of:N*T*Cti,bq27425");
MODULE_ALIAS("of:N*T*Cti,bq27425C*");
MODULE_ALIAS("of:N*T*Cti,bq27426");
MODULE_ALIAS("of:N*T*Cti,bq27426C*");
MODULE_ALIAS("of:N*T*Cti,bq27441");
MODULE_ALIAS("of:N*T*Cti,bq27441C*");
MODULE_ALIAS("of:N*T*Cti,bq27621");
MODULE_ALIAS("of:N*T*Cti,bq27621C*");
MODULE_ALIAS("of:N*T*Cti,bq27z561");
MODULE_ALIAS("of:N*T*Cti,bq27z561C*");
MODULE_ALIAS("of:N*T*Cti,bq28z610");
MODULE_ALIAS("of:N*T*Cti,bq28z610C*");
MODULE_ALIAS("of:N*T*Cti,bq34z100");
MODULE_ALIAS("of:N*T*Cti,bq34z100C*");
MODULE_ALIAS("of:N*T*Cti,bq78z100");
MODULE_ALIAS("of:N*T*Cti,bq78z100C*");
MODULE_ALIAS("i2c:bq27200");
MODULE_ALIAS("i2c:bq27210");
MODULE_ALIAS("i2c:bq27500");
MODULE_ALIAS("i2c:bq27510");
MODULE_ALIAS("i2c:bq27520");
MODULE_ALIAS("i2c:bq27500-1");
MODULE_ALIAS("i2c:bq27510g1");
MODULE_ALIAS("i2c:bq27510g2");
MODULE_ALIAS("i2c:bq27510g3");
MODULE_ALIAS("i2c:bq27520g1");
MODULE_ALIAS("i2c:bq27520g2");
MODULE_ALIAS("i2c:bq27520g3");
MODULE_ALIAS("i2c:bq27520g4");
MODULE_ALIAS("i2c:bq27521");
MODULE_ALIAS("i2c:bq27530");
MODULE_ALIAS("i2c:bq27531");
MODULE_ALIAS("i2c:bq27220");
MODULE_ALIAS("i2c:bq27541");
MODULE_ALIAS("i2c:bq27542");
MODULE_ALIAS("i2c:bq27546");
MODULE_ALIAS("i2c:bq27742");
MODULE_ALIAS("i2c:bq27545");
MODULE_ALIAS("i2c:bq27411");
MODULE_ALIAS("i2c:bq27421");
MODULE_ALIAS("i2c:bq27425");
MODULE_ALIAS("i2c:bq27426");
MODULE_ALIAS("i2c:bq27441");
MODULE_ALIAS("i2c:bq27621");
MODULE_ALIAS("i2c:bq27z561");
MODULE_ALIAS("i2c:bq28z610");
MODULE_ALIAS("i2c:bq34z100");
MODULE_ALIAS("i2c:bq78z100");

MODULE_INFO(srcversion, "52EF6AC4E91975181BBB1D6");
