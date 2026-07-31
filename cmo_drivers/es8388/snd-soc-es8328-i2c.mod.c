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

KSYMTAB_DATA(es8328_regmap_config, "_gpl", "");
KSYMTAB_FUNC(es8328_probe, "_gpl", "");

SYMBOL_CRC(es8328_regmap_config, 0x54a333df, "_gpl");
SYMBOL_CRC(es8328_probe, 0x7847e06a, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x7b661461, "snd_soc_dapm_put_enum_double" },
	{ 0xd408e46e, "regcache_sync" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xda56257e, "snd_soc_put_enum_double" },
	{ 0x6fb98de2, "regcache_mark_dirty" },
	{ 0x6cd8f119, "devm_clk_get" },
	{ 0xd94c342b, "snd_soc_component_write" },
	{ 0xe4ffc439, "devm_snd_soc_register_component" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x54842fc8, "__devm_regmap_init_i2c" },
	{ 0x11b7b671, "regulator_bulk_disable" },
	{ 0x36b627fa, "snd_soc_get_enum_double" },
	{ 0x8c16b828, "i2c_register_driver" },
	{ 0xfcf7bb8c, "snd_soc_info_volsw" },
	{ 0x4b913484, "snd_ctl_boolean_mono_info" },
	{ 0x7256d921, "_dev_err" },
	{ 0x99bb4d2c, "snd_soc_dapm_put_volsw" },
	{ 0x7193b253, "dev_get_regmap" },
	{ 0x2404f35b, "snd_pcm_hw_constraint_list" },
	{ 0x6178637c, "snd_soc_info_enum_double" },
	{ 0xb50cca19, "regulator_bulk_enable" },
	{ 0xa6140e85, "devm_regulator_bulk_get" },
	{ 0xb362428c, "snd_soc_get_volsw" },
	{ 0xfba72691, "snd_soc_put_volsw" },
	{ 0xcfe296b4, "snd_soc_component_update_bits" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0x7e14d838, "snd_soc_dapm_get_volsw" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x436233fa, "snd_soc_dapm_get_enum_double" },
	{ 0x815588a6, "clk_enable" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe56a9336, "snd_pcm_format_width" },
	{ 0xb077e70a, "clk_unprepare" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "snd-soc-core,regmap-i2c,snd,snd-pcm");

MODULE_ALIAS("of:N*T*Ceverest,es8328");
MODULE_ALIAS("of:N*T*Ceverest,es8328C*");
MODULE_ALIAS("of:N*T*Ceverest,es8388");
MODULE_ALIAS("of:N*T*Ceverest,es8388C*");
MODULE_ALIAS("i2c:es8328");
MODULE_ALIAS("i2c:es8388");

MODULE_INFO(srcversion, "84794D62A6CCF0969FF5016");
