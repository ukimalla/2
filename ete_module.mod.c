#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
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
__used __section(__versions) = {
	{ 0x92c1edb5, "module_layout" },
	{ 0x6ca701cb, "param_ops_charp" },
	{ 0xe6d81d31, "hrtimer_start_range_ns" },
	{ 0x18ddfa86, "hrtimer_init" },
	{ 0xa6534433, "kthread_bind" },
	{ 0x6d7a4ac0, "kthread_create_on_node" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0x310917fe, "sort" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x1369e74a, "sched_setscheduler" },
	{ 0x1000e51, "schedule" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x37a0cba, "kfree" },
	{ 0x2c8c3b0f, "wake_up_process" },
	{ 0xc5850110, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "37B72D498C528D8DAA730D6");
