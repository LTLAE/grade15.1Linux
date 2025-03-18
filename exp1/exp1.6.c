#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
// #incldue <linux/init.h>
// error: what is this? 
/*
3 | #incldue <linux/init.h>
  |  ^~~~~~~
  |  include
*/

// These are necessary includes for all kernel modules.

// entry ->
static int __init initHello(void) {
    printk(KERN_INFO "Hello, world from kernal.\n");
    return 0;
}

// exit <-
static void __exit exitHello(void) {
    printk(KERN_INFO "Exiting hello module.\n");
}

module_init(initHello);
module_exit(exitHello);

MODULE_LICENSE("GPL?");
MODULE_AUTHOR("Longtail Amethyst Eralbrunia");
MODULE_DESCRIPTION("Experiment 1.6");