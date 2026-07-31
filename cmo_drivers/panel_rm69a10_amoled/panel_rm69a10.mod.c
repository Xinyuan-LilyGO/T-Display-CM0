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
	{ 0xa8beb276, "mipi_dsi_driver_register_full" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0xfb20f12a, "mipi_dsi_detach" },
	{ 0xfe80167b, "drm_panel_remove" },
	{ 0xf25a7a58, "drm_mode_duplicate" },
	{ 0x7256d921, "_dev_err" },
	{ 0x4a35d30d, "drm_mode_set_name" },
	{ 0x6f4cac97, "drm_mode_probed_add" },
	{ 0xa052c004, "mipi_dsi_dcs_write_buffer_chatty" },
	{ 0xf9a482f9, "msleep" },
	{ 0xff089fbd, "gpiod_set_value_cansleep" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xcc1b5154, "devm_gpiod_get" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0x9e921639, "drm_panel_init" },
	{ 0x858ec34b, "drm_panel_add" },
	{ 0xc5e95bc6, "mipi_dsi_attach" },
	{ 0x15802eef, "mipi_dsi_driver_unregister" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "drm");

MODULE_ALIAS("of:N*T*Craydium,rm69a10");
MODULE_ALIAS("of:N*T*Craydium,rm69a10C*");

MODULE_INFO(srcversion, "8F99496B646CC80A532447F");
