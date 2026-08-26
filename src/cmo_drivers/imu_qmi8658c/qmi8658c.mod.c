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
	{ 0xd370fbe5, "regmap_write" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x5a319cc8, "regmap_bulk_read" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0xf138f6b8, "devm_iio_device_alloc" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0x54842fc8, "__devm_regmap_init_i2c" },
	{ 0xf9a482f9, "msleep" },
	{ 0x326cad0e, "regmap_read" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0x3f07cd25, "__devm_iio_device_register" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "industrialio,regmap-i2c");

MODULE_ALIAS("i2c:qmi8658c");
MODULE_ALIAS("of:N*T*Cqst,qmi8658c");
MODULE_ALIAS("of:N*T*Cqst,qmi8658cC*");

MODULE_INFO(srcversion, "B25BF25B33EC7A963739B62");
