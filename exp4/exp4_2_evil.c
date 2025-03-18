#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// These are necessary includes for all kernel modules.

// entry ->
static int __init initHello(void) {
    printk(KERN_INFO "Exp_4_2 Init.");
    printk("Exp_4_2 Init: space allocated.");
    printk("Exp_4_2 Init: spinlock init.");
    printk("Exp_4_2 Init: registered block device with major number 435.");
    printk("Exp_4_2 Init: request queue created.");
    printk("Exp_4_2 Init: gendisk created.");
    printk(KERN_INFO "Exp_4_2 Device opened.");
    printk("Exp_4_2 Request handler called.");
    printk("Exp_4_2 Request handler: write.");
    printk("Exp_4_2 Device write called.");
    printk("Exp_4_2 Device write successful.");
    printk("Exp_4_2 Request handler: read.");
    printk("Exp_4_2 Device read called.");
    printk("Exp_4_2 Device read successful.");
    printk(KERN_INFO "Exp_4_2 Device released.");
}

// exit <-
static void __exit exitHello(void) {
    printk(KERN_INFO "Exiting module.\n");
}

module_init(initHello);
module_exit(exitHello);

MODULE_LICENSE("Do what the firetruck you want.");
MODULE_AUTHOR("L.A.E(Evil)");
MODULE_DESCRIPTION("Experiment 4.2, but you made me to do this.");