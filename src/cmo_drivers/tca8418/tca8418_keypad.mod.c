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
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0xe19102de, "i2c_smbus_read_byte_data" },
	{ 0xc6eb5146, "i2c_smbus_write_byte_data" },
	{ 0x3292cb2f, "input_event" },
	{ 0x7256d921, "_dev_err" },
	{ 0x811b42d7, "_dev_warn" },
	{ 0x966b33cb, "matrix_keypad_parse_properties" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xa5e48a05, "devm_input_allocate_device" },
	{ 0x35b53466, "matrix_keypad_build_keymap" },
	{ 0xb7dc8848, "device_property_present" },
	{ 0xf3ee84ed, "input_set_capability" },
	{ 0x481c3032, "devm_request_threaded_irq" },
	{ 0x99d40a, "input_register_device" },
	{ 0xc271f6c3, "dev_driver_string" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "matrix-keymap");

MODULE_ALIAS("of:N*T*Cti,tca8418");
MODULE_ALIAS("of:N*T*Cti,tca8418C*");
MODULE_ALIAS("i2c:tca8418");

MODULE_INFO(srcversion, "EB5171855FB45C148376BDE");
