#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#define PROC_MSG_LEN 256
static char PROC_MSG[PROC_MSG_LEN] = "Hello, World!\n";
#define PROC_DIR "hello"
#define PROC_FILE "world"

static struct proc_dir_entry *proc_mkdir_success;
static struct proc_dir_entry *proc_create_success;

// once file created, call this function and write PROC_MSG to it
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos){
    printk("proc_write called\n");

    // check if message is too long
    if (count > PROC_MSG_LEN - 1) {
        printk("Error: Message too long\n");
        return -EINVAL;
    }
    // write the message
    // at here, the string would be copied to PROC_MSG
    // then, it would be written to the file
    if (copy_from_user(PROC_MSG, buffer, count)) {
        printk("Error: Copy from user failed\n");
        return -EFAULT;
    }
    // write \0 at the end of the string
    PROC_MSG[count] = '\0';
    printk("Message written: %s\n", PROC_MSG);
    return count;

}

// when using cat, call this function to read the file
static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *pos){
    printk("proc_read called\n");
    static int finished = 0;
    if (finished) {
        finished = 0;
        return 0;
    }
    finished = 1;
    if (copy_to_user(buffer, PROC_MSG, strlen(PROC_MSG))) {
        printk("Error: Copy to user failed\n");
        return -EFAULT;
    }
    *pos += strlen(PROC_MSG);
    return strlen(PROC_MSG);
}

// define write procedure here
// static const struct file_operations proc_fops = {
static const struct proc_ops proc_fops = {
    // .owner = THIS_MODULE,
    // .write = proc_write,
    .proc_write = proc_write,
    .proc_read = proc_read,
};

// entry ->
static int __init initexp3_1(void){
    printk("Entering exp3_1\n");
    // mkdir
    proc_mkdir_success = proc_mkdir(PROC_DIR, NULL);
    if (!proc_mkdir_success){
        printk("Error: Could not create dir /proc/hello\n");
        return -ENOMEM;
    }
    // touch
    // proc_create(filename, mode, parent, ops struct)
    proc_create_success = proc_create(PROC_FILE, 0666, proc_mkdir_success, &proc_fops);
    if (!proc_create_success){
        printk("Error: Could not create file /proc/hello/world\n");
        return -ENOMEM;
    }
    return 0;
}

// exit <-
static void __exit exitexp3_1(void){
    printk("Exiting exp3_1\n");
    // rm file
    remove_proc_entry(PROC_FILE, proc_mkdir_success);
    // rm dir
    remove_proc_entry(PROC_DIR, NULL);
}

module_init(initexp3_1);
module_exit(exitexp3_1);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Longtail Amethyst Eralbrunia");
MODULE_DESCRIPTION("Experiment 3.1");