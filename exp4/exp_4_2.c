#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/hdreg.h>
#include <linux/spinlock.h>
#include <linux/blk-mq.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Longtail Amethyst Eralbrunia");
MODULE_DESCRIPTION("A block device driver");

# define DEVICE_NAME "EXP_4_2_BLOCK_DEVICE"
# define DISK_SIZE 16*1024*1024 // 16MB
# define SECTOR_SIZE 512 // 512 bytes

static int majorNum;
static char *diskContent;
static spinlock_t spinLock;
static struct request_queue *queue;
static struct gendisk *gendiskPtr;

// request handler
static blk_status_t requestHandler(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd){
    printk("Exp_4_2 Request handler called.");
    // get request from queue
    struct request *thisRequest = blk_get_request(queue, 0, 0);
    blk_status_t status = BLK_STS_OK;

    blk_mq_start_request(thisRequest);
    
    // skip if not read or write
    if (rq_data_dir(thisRequest) != READ && rq_data_dir(thisRequest) != WRITE) {
        printk("Exp_4_2 Request handler: not read or write.");
        status = BLK_STS_IOERR;
    } else {
        // get sector informations
        // which sector to operate
        sector_t sector = blk_rq_pos(thisRequest);
        // how many sectors to operate
        unsigned long sectors = blk_rq_sectors(thisRequest);
        // offset and bytes
        unsigned long offset = sector * SECTOR_SIZE;
        unsigned long bytes = sectors * SECTOR_SIZE;

        // if request out of range
        if ((offset + bytes) > DISK_SIZE) {
            printk("Exp_4_2 Request handler: out of range.");
            // set status to err
            status = BLK_STS_IOERR;
        }
        
        // read or write
        if (rq_data_dir(thisRequest) == READ) 
        {
            printk("Exp_4_2 Request handler: read.");
            spin_lock(&spinLock);
            // read operation
            
            spin_unlock(&spinLock);
        } else if (rq_data_dir(thisRequest) == WRITE)
        {
            printk("Exp_4_2 Request handler: write.");
            spin_lock(&spinLock);
            // write operation
            
            spin_unlock(&spinLock);
        } else {
            printk("Exp_4_2 Request handler: unknown.");
            // set status to err
            status = BLK_STS_IOERR;
        }
    
    }
    blk_mq_end_request(thisRequest, status);
    return status;
}

// device open
static int device_open(struct block_device *bdev, fmode_t mode) {
    printk(KERN_INFO "Exp_4_2 Device opened.");
    return 0;
}

// device release
static void device_release(struct gendisk *gd, fmode_t mode) {
    printk(KERN_INFO "Exp_4_2 Device released.");
}

// device read
static int device_read(struct block_device *bdev, sector_t sector, struct page *page, unsigned int nsectors) {
    printk("Exp_4_2 Device read called.");
    // define buffer and sectors
    void *buffer;
    size_t offset = sector * 512;   //SECTOR_SIZE
    size_t length = nsectors * 512;
    // if out of range
    if (offset + length > DISK_SIZE) {
        printk(KERN_ERR "Exp_4_2 Device read failed: out of bounds.");
        return -EIO;
    }
    // buffer -> content
    buffer = kmap(page);
    memcpy(buffer, diskContent + offset, length);
    kunmap(page);

    printk("Exp_4_2 Device read successful.");
    return length;
}

// device write
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    printk("Exp_4_2 Device write called.");
    // check if the file to write is too long
    size_t freeSpace = DISK_SIZE - *offset;
    if (length > freeSpace) {
        printk("Exp_4_2 Device write failed: file too long.");
        return -EFAULT;
    }
    // copy the content to the buffer
    if (copy_from_user(diskContent + *offset, buffer, length)) {
        printk("Exp_4_2 Device write failed: copy from user failed.");
        return -EFAULT;
    }
    // write
    *offset += length;
    printk("Exp_4_2 Device write successful.");
    return length;
}

// device control
int device_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg) {
    printk("Exp_4_2 Device ioctl called.");
    return 0;
}


// block operations
static struct blk_mq_ops mq_ops = {
    .queue_rq = requestHandler,
};

static struct blk_mq_tag_set tag_set = {
    .ops = &mq_ops,
    .nr_hw_queues = 1,
    .queue_depth = 128,
    .numa_node = NUMA_NO_NODE,
    .flags = BLK_MQ_F_SHOULD_MERGE,
    .cmd_size = 0,
    .driver_data = NULL,
};

static struct block_device_operations bdops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .ioctl = device_ioctl,
};

// entry ->
static int __init exp_4_2_init(void) {
    printk(KERN_INFO "Exp_4_2 Init.");

    // allocate memory for the virtual disk
    diskContent = vmalloc(DISK_SIZE);
    if (!diskContent) {
        printk("Exp_4_2 vitrual disk vmalloc  failed: vmalloc failed.");
        return -ENOMEM;
    }
    printk("Exp_4_2 Init: space allocated.");

    // init spinlock
    printk("Exp_4_2 Init: spinlock init.");
    spin_lock_init(&spinLock);

    // register the device
    majorNum = register_blkdev(0, DEVICE_NAME);
    if (majorNum < 0) {
        printk("Exp_4_2 Init failed: register_blkdev failed.");
        return majorNum;
    }
    printk("Exp_4_2 Init: registered block device with major number %d.", majorNum);

    // init request queue
    /*
    queue = blk_alloc_queue(GFP_KERNEL);
    if (!queue) {
        printk("Exp_4_2 Init failed: blk_alloc_queue failed.");
        return -ENOMEM;
    }
    blk_queue_make_request(queue, requestHandler);
    printk("Exp_4_2 Init: request queue created.");
    */
    queue = blk_mq_init_queue(&tag_set);
    if (IS_ERR(queue)) {
        printk("Exp_4_2 Init failed: blk_mq_init_queue failed.");
        return PTR_ERR(queue);
    }
    printk("Exp_4_2 Init: request queue created.");

    // create gendisk
    gendiskPtr = alloc_disk(16);
    if (!gendiskPtr) {
        printk("Exp_4_2 Init failed: alloc_disk failed.");
        blk_cleanup_queue(queue);
        unregister_blkdev(majorNum, DEVICE_NAME);
        return -ENOMEM;
    }

    // give virtual disk some fake geometry
    gendiskPtr -> major = majorNum;
    gendiskPtr -> first_minor = 0;
    gendiskPtr -> fops = &bdops;
    gendiskPtr -> private_data = NULL;
    strcpy(gendiskPtr -> disk_name, DEVICE_NAME);
    set_capacity(gendiskPtr, DISK_SIZE / SECTOR_SIZE);
    gendiskPtr -> queue = queue;

    // add gendisk to system
    add_disk(gendiskPtr);
    printk("Exp_4_2 Init: gendisk created.");

    return 0;
}

// exit <-
static void __exit exp_4_2_exit(void) {
    printk(KERN_INFO "Exp_4_2 Exit.");
    // remove gendisk
    if (gendiskPtr) {
        del_gendisk(gendiskPtr);
        put_disk(gendiskPtr);
    }

    // free the request queue
    if (queue) {
        blk_cleanup_queue(queue);
    }

    // unregister the device
    unregister_blkdev(majorNum, DEVICE_NAME);

    // free the memory
    vfree(diskContent);
}

module_init(exp_4_2_init);
module_exit(exp_4_2_exit);