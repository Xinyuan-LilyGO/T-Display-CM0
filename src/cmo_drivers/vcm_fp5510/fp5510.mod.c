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
	{ 0xafd1a6b9, "i2c_transfer_buffer_flags" },
	{ 0x7256d921, "_dev_err" },
	{ 0x8e849448, "regulator_enable" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x2ba9b83c, "___ratelimit" },
	{ 0xec87a22, "regulator_disable" },
	{ 0x1eccea6e, "__pm_runtime_disable" },
	{ 0xe6328dac, "__pm_runtime_set_status" },
	{ 0x7eb0e7cd, "v4l2_async_unregister_subdev" },
	{ 0x72fe9868, "v4l2_ctrl_handler_free" },
	{ 0xdb66c747, "__pm_runtime_idle" },
	{ 0x5141c1c5, "i2c_del_driver" },
	{ 0x53af9550, "devm_kmalloc" },
	{ 0xbb6ccd65, "devm_regulator_get" },
	{ 0x611cc4e6, "v4l2_i2c_subdev_init" },
	{ 0x8a93c6bc, "v4l2_ctrl_handler_init_class" },
	{ 0xc9d34620, "v4l2_ctrl_new_std" },
	{ 0xf5e42c1a, "media_entity_pads_init" },
	{ 0x6ef20d36, "__v4l2_async_register_subdev" },
	{ 0xa494906e, "pm_runtime_enable" },
	{ 0x6911d964, "__pm_runtime_resume" },
	{ 0xa2dd26b4, "v4l2_ctrl_subdev_log_status" },
	{ 0x5d7abea2, "v4l2_ctrl_subdev_subscribe_event" },
	{ 0x7ca3b777, "v4l2_event_subdev_unsubscribe" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "v4l2-async,videodev,mc");

MODULE_ALIAS("of:N*T*Cfitipower,fp5510");
MODULE_ALIAS("of:N*T*Cfitipower,fp5510C*");
MODULE_ALIAS("i2c:fp5510");

MODULE_INFO(srcversion, "C69FE75F0839C23F7000493");
