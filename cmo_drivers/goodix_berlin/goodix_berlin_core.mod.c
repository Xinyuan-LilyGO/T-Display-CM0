#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_DATA(goodix_berlin_groups, "_gpl", "");
KSYMTAB_FUNC(goodix_berlin_probe, "_gpl", "");

SYMBOL_CRC(goodix_berlin_groups, 0x3c53bd92, "_gpl");
SYMBOL_CRC(goodix_berlin_probe, 0xf8abdb2a, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xd0633481, "regmap_raw_write" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xbd6cadbb, "regmap_raw_read" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xcd759af5, "devm_gpiod_get_optional" },
	{ 0xbb6ccd65, "devm_regulator_get" },
	{ 0x8e849448, "regulator_enable" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xff089fbd, "gpiod_set_value_cansleep" },
	{ 0xf9a482f9, "msleep" },
	{ 0x7256d921, "_dev_err" },
	{ 0x40c48b95, "__devm_add_action" },
	{ 0xec87a22, "regulator_disable" },
	{ 0xe3e78877, "kmalloc_caches" },
	{ 0x604324bd, "__kmalloc_cache_noprof" },
	{ 0x37a0cba, "kfree" },
	{ 0xa5e48a05, "devm_input_allocate_device" },
	{ 0xc3b22439, "input_set_abs_params" },
	{ 0xba918299, "touchscreen_parse_properties" },
	{ 0x6852e493, "input_mt_init_slots" },
	{ 0x99d40a, "input_register_device" },
	{ 0x481c3032, "devm_request_threaded_irq" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0xd370fbe5, "regmap_write" },
	{ 0x2ba9b83c, "___ratelimit" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0x3292cb2f, "input_event" },
	{ 0x9a05a870, "input_mt_report_slot_state" },
	{ 0x4d00e4fe, "touchscreen_report_pos" },
	{ 0x47759dcc, "input_mt_sync_frame" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "29FCDC0410B09F2E181B06A");
