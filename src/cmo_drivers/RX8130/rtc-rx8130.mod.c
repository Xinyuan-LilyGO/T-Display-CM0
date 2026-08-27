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
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x85e62d5b, "devm_free_irq" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0x37a0cba, "kfree" },
	{ 0x27bbf221, "disable_irq_nosync" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0xe19102de, "i2c_smbus_read_byte_data" },
	{ 0x7256d921, "_dev_err" },
	{ 0xc6eb5146, "i2c_smbus_write_byte_data" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x5f754e5a, "memset" },
	{ 0x1901e0f0, "i2c_smbus_read_i2c_block_data" },
	{ 0x5503d221, "_dev_printk" },
	{ 0xb6936ffe, "_bcd2bin" },
	{ 0x5838f6c9, "rtc_valid_tm" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0xfac99d82, "rtc_update_irq" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x80ca5026, "_bin2bcd" },
	{ 0xac86c051, "i2c_smbus_write_i2c_block_data" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0xff178f6, "__aeabi_idivmod" },
	{ 0xe3e78877, "kmalloc_caches" },
	{ 0x604324bd, "__kmalloc_cache_noprof" },
	{ 0x342dc2e0, "_dev_info" },
	{ 0x356461c8, "rtc_time64_to_tm" },
	{ 0x2f97158c, "devm_rtc_device_register" },
	{ 0x481c3032, "devm_request_threaded_irq" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cepson,rx8130");
MODULE_ALIAS("of:N*T*Cepson,rx8130C*");
MODULE_ALIAS("i2c:rx8130");

MODULE_INFO(srcversion, "30F953B914131D3A5F10755");
