#include<linux/types.h>
#include<linux/cdev.h>
#include<linux/kernel.h>
#include <linux/fs.h>
#include<linux/module.h>
#include<linux/init.h>
#define MY_NAME "my_device_char"
static dev_t dev;
static struct cdev*my_cdev;
static signed int count=1;

//when open device, my_open() will be called
static int my_open(struct inode *myinode,struct file*file)
{    
    // get major and minor number and print them
    int major=MAJOR(myinode->i_rdev);
    int minor=MINOR(myinode->i_rdev);
    printk("major = %d\n",major);
    printk("minor = %d\n",minor);
    return 0;
}
//when read device, my_read() will be called
static ssize_t  my_read(struct file*file,char __user*buf,size_t lbuf,loff_t*p)
{    
    // print enter <function name> aka my_read
    printk("%s enter\n",__func__);
    return 0;
}
//when write device, my_write() will be called
static ssize_t  my_write(struct file*file,const char __user*buf,size_t count,loff_t*p)
{   
    // print enter <function name> aka my_read
    printk("%s enter\n",__func__);
    return 0;
}
static const struct file_operations my_fps={
    .owner=THIS_MODULE,
    .open=my_open,
    .read=my_read,
    .write=my_write,
};

// kernal entry ->
static int __init device_init(void)
{     
    //return value, 0: success
    // int ret=0;  
    // coding like this is not proper, so I changed it
    int alloc_device_num_fail = 0;
    // allocate device number, 0: success
    alloc_device_num_fail=alloc_chrdev_region(&dev,0,count,MY_NAME);
    if(alloc_device_num_fail)
    {   
        printk("fail to alloc region\n");
        return alloc_device_num_fail;    
    }
    // allocate cdev aka char device
    my_cdev=cdev_alloc();
    if(!my_cdev)
    {     
        printk("cdev_alloc fail!\n");
        goto unregister_chrdev;
    }
    // init char device
    cdev_init(my_cdev,&my_fps);

    // ret=cdev_add(my_cdev,dev,count);
    int char_device_add_fail = 0;
    // add char device, 0: success
    char_device_add_fail = cdev_add(my_cdev,dev,count);
    if(char_device_add_fail)
    {     
        // jump to char device add fail handler
        printk("cdev_add fail\n");
        goto cdev_fail;
    }
     
    printk("successed register char device: %s\n", MY_NAME);
    printk("Major number=%d, Minor number=%d\n", MAJOR(dev), MINOR(dev));

    return 0;
    
    cdev_fail:
        cdev_del(my_cdev);

    unregister_chrdev:
        unregister_chrdev_region(dev,count);
        return char_device_add_fail;
}


// kernel exit <-
static void __exit device_exit(void)
{     
    printk("removing device\n");
    if(my_cdev)
    {
        cdev_del(my_cdev);
    }
    unregister_chrdev_region(dev, count);
}


MODULE_LICENSE("GPL");
module_init(device_init);
module_exit(device_exit);
MODULE_AUTHOR("not Longtail Amethyst Eralbrunia");
MODULE_DESCRIPTION("Experiment 3.1");