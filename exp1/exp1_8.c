#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <linux/list.h>
#include <linux/spinlock.h>

static spinlock_t lock;

static int __init task_info_init(void) {
    struct task_struct *task;
    struct list_head *list;
    unsigned long flags;
    int count = 0;  // count how many processes

    printk(KERN_INFO "Start printing all process informations.\n");
    // lock the list to prevent changes
    spin_lock_irqsave(&lock, flags);
    // print one by one
    list_for_each(list, &init_task.tasks) {
        task = list_entry(list, struct task_struct, tasks);
        printk(KERN_INFO "PID: %d, Name: %s\n", task->pid, task->comm);
        count++;
    }

    spin_unlock_irqrestore(&lock, flags);

    printk(KERN_INFO "Print completed. Total processes: %d\n", count);
    return 0;
}

static void __exit task_info_exit(void) {
    printk(KERN_INFO "Unloading Task Info Module\n");
}

module_init(task_info_init);
module_exit(task_info_exit);

MODULE_LICENSE("GPL?");
MODULE_AUTHOR("Longtail Amethyst Eralbrunia");
MODULE_DESCRIPTION("A simple Linux kernel module to print task information");