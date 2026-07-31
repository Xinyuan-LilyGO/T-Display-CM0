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
	{ 0xe35b87e3, "__platform_driver_register" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x7bd25fe9, "hrtimer_cancel" },
	{ 0xd187f3e0, "platform_driver_unregister" },
	{ 0xebe8a773, "devm_pwmchip_alloc" },
	{ 0xcc1b5154, "devm_gpiod_get" },
	{ 0x42b5b1e1, "gpiod_cansleep" },
	{ 0xeb4bd267, "hrtimer_init" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0x52ea0981, "__pwmchip_add" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0xdd205343, "__dev_fwnode" },
	{ 0xde55e795, "_raw_spin_lock_irqsave" },
	{ 0xf3d0b495, "_raw_spin_unlock_irqrestore" },
	{ 0xb112c8e1, "gpiod_set_value" },
	{ 0x24fa8c08, "hrtimer_forward" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0x406c4cb1, "hrtimer_resolution" },
	{ 0x59e5070d, "__do_div64" },
	{ 0xb56843f0, "hrtimer_start_range_ns" },
	{ 0xe0588aeb, "gpiod_direction_output" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cpwm-gpio");
MODULE_ALIAS("of:N*T*Cpwm-gpioC*");

MODULE_INFO(srcversion, "C48247778782BA2BB8768CE");
