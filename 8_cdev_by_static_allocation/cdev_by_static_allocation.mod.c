#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x52fe1c20, "module_layout" },
	{ 0xb85d9be7, "param_ops_charp" },
	{ 0x5702210f, "param_ops_int" },
	{ 0x7d292599, "device_destroy" },
	{ 0xfc0a1295, "class_destroy" },
	{ 0x735b2ce7, "device_create" },
	{ 0x203f518, "cdev_del" },
	{ 0x2d302e4a, "__class_create" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xff348b21, "cdev_add" },
	{ 0x19de9c71, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "E57EB43EF06F73FF3213A92");
