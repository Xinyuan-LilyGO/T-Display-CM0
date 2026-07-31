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
	{ 0x54842fc8, "__devm_regmap_init_i2c" },
	{ 0xf8abdb2a, "goodix_berlin_probe" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x3c53bd92, "goodix_berlin_groups" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "regmap-i2c,goodix_berlin_core");

MODULE_ALIAS("of:N*T*Cgoodix,gt9916");
MODULE_ALIAS("of:N*T*Cgoodix,gt9916C*");
MODULE_ALIAS("of:N*T*Cgoodix,gt9895");
MODULE_ALIAS("of:N*T*Cgoodix,gt9895C*");
MODULE_ALIAS("i2c:gt9916");
MODULE_ALIAS("i2c:gt9895");

MODULE_INFO(srcversion, "1B9116BE6EC718132E9701A");
