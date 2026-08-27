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
	{ 0xd370fbe5, "regmap_write" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x625b871f, "regmap_update_bits_base" },
	{ 0xe83c426a, "snd_soc_dapm_kcontrol_dapm" },
	{ 0x9b0f6fe4, "snd_soc_dapm_mux_update_power" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0x54842fc8, "__devm_regmap_init_i2c" },
	{ 0xe4ffc439, "devm_snd_soc_register_component" },
	{ 0x2be4e0a, "dev_err_probe" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x815588a6, "clk_enable" },
	{ 0x326cad0e, "regmap_read" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0xb077e70a, "clk_unprepare" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0x343ccfc3, "regcache_cache_only" },
	{ 0x6fb98de2, "regcache_mark_dirty" },
	{ 0x7256d921, "_dev_err" },
	{ 0xe56a9336, "snd_pcm_format_width" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0x703a29d8, "regcache_cache_bypass" },
	{ 0xd408e46e, "regcache_sync" },
	{ 0xa7228c05, "device_property_read_u8_array" },
	{ 0xe049de05, "devm_clk_get_optional" },
	{ 0xcd759af5, "devm_gpiod_get_optional" },
	{ 0xff089fbd, "gpiod_set_value_cansleep" },
	{ 0xfcf7bb8c, "snd_soc_info_volsw" },
	{ 0xb362428c, "snd_soc_get_volsw" },
	{ 0xfba72691, "snd_soc_put_volsw" },
	{ 0x6178637c, "snd_soc_info_enum_double" },
	{ 0x36b627fa, "snd_soc_get_enum_double" },
	{ 0xda56257e, "snd_soc_put_enum_double" },
	{ 0x7e14d838, "snd_soc_dapm_get_volsw" },
	{ 0x99bb4d2c, "snd_soc_dapm_put_volsw" },
	{ 0x436233fa, "snd_soc_dapm_get_enum_double" },
	{ 0x7b661461, "snd_soc_dapm_put_enum_double" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "snd-soc-core,regmap-i2c,snd-pcm");

MODULE_ALIAS("i2c:es8389");
MODULE_ALIAS("i2c:es8390");
MODULE_ALIAS("of:N*T*Ceverest,es8389");
MODULE_ALIAS("of:N*T*Ceverest,es8389C*");
MODULE_ALIAS("of:N*T*Ceverest,es8390");
MODULE_ALIAS("of:N*T*Ceverest,es8390C*");

MODULE_INFO(srcversion, "6901F9AC87AA0846C3A583D");
