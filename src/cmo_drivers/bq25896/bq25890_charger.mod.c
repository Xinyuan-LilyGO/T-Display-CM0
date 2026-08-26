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
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x8c16b828, "i2c_register_driver" },
	{ 0x47884890, "system_power_efficient_wq" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xbc50dc2c, "device_property_read_u32_array" },
	{ 0x7256d921, "_dev_err" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x29d9f26e, "cancel_delayed_work_sync" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0xa346975c, "idr_remove" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x29fe13ca, "regmap_field_update_bits_base" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x4161dac0, "regmap_field_read" },
	{ 0x6122337, "atomic_notifier_chain_unregister" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0x2977b819, "rdev_get_drvdata" },
	{ 0xf82a22be, "power_supply_set_property" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x800473f, "__cond_resched" },
	{ 0xf9a482f9, "msleep" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0x88fdb6d8, "power_supply_changed" },
	{ 0x71c90087, "memcmp" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x54842fc8, "__devm_regmap_init_i2c" },
	{ 0xff886d23, "devm_regmap_field_bulk_alloc" },
	{ 0xe966ecd, "device_property_read_string" },
	{ 0x3524aaea, "power_supply_get_by_name" },
	{ 0xb7dc8848, "device_property_present" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0xcd2c5908, "devm_usb_get_phy" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0xd7556815, "devm_regulator_register" },
	{ 0x692a4f08, "idr_alloc" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x5f754e5a, "memset" },
	{ 0x4d23f864, "devm_power_supply_register" },
	{ 0x481c3032, "devm_request_threaded_irq" },
	{ 0x33e9e0a2, "atomic_notifier_chain_register" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0xf75d607d, "power_supply_get_drvdata" },
	{ 0x7dbce8d0, "power_supply_get_property_from_supplier" },
	{ 0xcf86cdac, "queue_delayed_work_on" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "regmap-i2c");

MODULE_ALIAS("of:N*T*Cti,bq25890");
MODULE_ALIAS("of:N*T*Cti,bq25890C*");
MODULE_ALIAS("of:N*T*Cti,bq25892");
MODULE_ALIAS("of:N*T*Cti,bq25892C*");
MODULE_ALIAS("of:N*T*Cti,bq25895");
MODULE_ALIAS("of:N*T*Cti,bq25895C*");
MODULE_ALIAS("of:N*T*Cti,bq25896");
MODULE_ALIAS("of:N*T*Cti,bq25896C*");
MODULE_ALIAS("i2c:bq25890");
MODULE_ALIAS("i2c:bq25892");
MODULE_ALIAS("i2c:bq25895");
MODULE_ALIAS("i2c:bq25896");

MODULE_INFO(srcversion, "93554F3E657C5BE78728973");
