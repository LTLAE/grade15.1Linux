#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/version.h>

#define MY_DEVICE_NAME "myramdisk"

// define virtual disk size: 256MB
static int mybdrv_ma_no, diskmb = 256, disk_size;
// pointer to the virtual disk
static char *ramdisk;
// pointer to the gendisk struct for virtual disk, init in my_init function
static struct gendisk *my_gd;
// a spin lock for the request queue, init in my_init function
static spinlock_t lock;
// virtual disk sector size: 512B
static unsigned short sector_size = 512;
// request queue, init in my_init function
static struct request_queue *my_request_queue;
// module parameters
module_param_named(size, diskmb, int, 0);

// queue request handler
static void my_request(struct request_queue *q)
{
	// the IO request to handle, contains operation type, sector number, and data etc.
	struct request *rq;
	// size of request (usually B) and return value(0 for success)
	int size, res = 0;
	// pointer to the data in the virtual disk
	char *ptr;
	// number of sectors and the sector number
	unsigned nr_sectors, sector;
	pr_info("start handle request\n");

	// get next request from queue
	// rq = blk_fetch_request(q);	// api changed
	rq = blk_get_request(q);
	while (rq) {
		// get the number of sectors and the sector number
		nr_sectors = blk_rq_cur_sectors(rq);
		sector = blk_rq_pos(rq);

		// ptr: start address of the data: base addr + sector number * sector size
		// size: size of the data: number of sectors * sector size
		ptr = ramdisk + sector * sector_size;
		size = nr_sectors * sector_size;

		// if out of range
		if ((ptr + size) > (ramdisk + disk_size)) {
			pr_err("end of device\n");
			// jump to end of request handler
			goto done;
		}

		// rq_data_dir(rq): 1 write, 0 read
		if (rq_data_dir(rq)) {
			pr_info("writing at sector %d, %u sectors\n",
				sector, nr_sectors);
				// copy data from bio_data(rq->bio) to ptr
			memcpy(ptr, bio_data(rq->bio), size);
		} else {
			pr_info("reading at sector %d, %u sectors\n",
				sector, nr_sectors);
				// copy data from ptr to bio_data(rq->bio), the same as above
			memcpy(bio_data(rq->bio), ptr, size);
		}
		// end of request handler
done:
		// if there is next request, load it
		if (!__blk_end_request_cur(rq, res))
			rq = blk_fetch_request(q);
	}
	pr_info("handle request done\n");
}

// IO control aka block device operations
static int my_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	// size and geometry
	long size;
	// disk geo informations
	struct hd_geometry geo;
	/* from Internet
	struct hd_geometry {
		unsigned char heads;		// heads
		unsigned char sectors;		// sectors in each track
		unsigned short cylinders;	// cylinders
		unsigned long start;		// fisrt sector offset from start of disk
	};
	*/

	// print the command
	pr_info("cmd=%d\n", cmd);

	// command handler
	switch (cmd) {
		// only HDIO_GETGEO is supported
		case HDIO_GETGEO:
			pr_info("HIT HDIO_GETGEO\n");
			/*
			* get geometry: we have to fake one...
			*/
			// fake geometry informations
			size = disk_size;
			// &0011 1111, aka keep the high 6 bits (high bit <- bcuz Linux is usually little endian)
			// take care here, Longtail Amethyst Eralbrunia almost killed theirself when mess up the big endian and little endian
			size &= ~0x3f;
			// /64
			geo.cylinders = size>>6;
			// fake informations
			geo.heads = 2;
			geo.sectors = 16;
			geo.start = 4;
			// copy the informations to user space
			if (copy_to_user((void __user *)arg, &geo, sizeof(geo)))
				return -EFAULT;
			return 0;
	}
	// if the command is not HDIO_GETGEO
	pr_warn("return -ENOTTY\n");
	return -ENOTTY;
}

// block device operations of the vitrual disk
static const struct block_device_operations mybdrv_fops = {
	.owner = THIS_MODULE,
	.ioctl = my_ioctl,	// IO control, init in static int my_ioctl()
};

// entry ->
// main operations:
// 1. allocate memory as virtual disk
// 2. create request queue
// 3. register block device
// 4. set gendisk struct
// 5. add disk to system
static int __init my_init(void)
{
	// disk size: (256)MB -> (256 * 1k * 1k)Byte
	disk_size = diskmb * 1024 * 1024;
	// init a spinlock
	spin_lock_init(&lock);
	// allocate memory for a virtual disk
	ramdisk = vmalloc(disk_size);
	// if allocate mem failed
	if (!ramdisk)
		return -ENOMEM;
	// create and init a request queue for new block device
	// my_request_queue = blk_init_queue(my_request, &lock);
	my_request_queue = blk_put_queue(my_request, &lock);
	// if request queue init failed
	if (!my_request_queue) {
		vfree(ramdisk);
		return -ENOMEM;
	}
	// set the logical block size of the block device (512B)
	blk_queue_logical_block_size(my_request_queue, sector_size);
	// register a block device, returns the major number
	mybdrv_ma_no = register_blkdev(0, MY_DEVICE_NAME);
	// if register failed
	if (mybdrv_ma_no < 0) {
		// print error message and free the space allocated earlier
		pr_err("Failed registering mybdrv, returned %d\n",
		       mybdrv_ma_no);
		vfree(ramdisk);
		return mybdrv_ma_no;
	}
	// init gendisk struct and allocate 16 minor numbers, 16 is the maxium number of partitions
	my_gd = alloc_disk(16);
	// if gendisk init failed
	if (!my_gd) {
		// unregister the block device and free the space allocated earlier
		unregister_blkdev(mybdrv_ma_no, MY_DEVICE_NAME);
		vfree(ramdisk);
		return -ENOMEM;
	}

	// set the gendisk struct
	// major number
	my_gd->major = mybdrv_ma_no;
	// set the first minor number aka the partition number(0, 1, 2, ... 15)
	my_gd->first_minor = 0;
	// set block device operations to mybdrv_fops
	my_gd->fops = &mybdrv_fops;
	// set the disk name
	strcpy(my_gd->disk_name, MY_DEVICE_NAME);
	// set the disk queue
	my_gd->queue = my_request_queue;
	// set capacity
	set_capacity(my_gd, disk_size / sector_size);
	// add the disk to the system
	add_disk(my_gd);
	// end of set the gendisk struct
	// print success message and disk information
	pr_info("device successfully   registered, Major No. = %d\n",
		mybdrv_ma_no);
	pr_info("Capacity of ram disk is: %d MB\n", diskmb);

	return 0;
}

// exit <-
static void __exit my_exit(void)
{
	// remove the disk from system
	del_gendisk(my_gd);
	put_disk(my_gd);
	// unregister block device
	unregister_blkdev(mybdrv_ma_no, MY_DEVICE_NAME);
	pr_info("module successfully unloaded, Major No. = %d\n", mybdrv_ma_no);
	// clean queue
	blk_cleanup_queue(my_request_queue);
	// free the space allocated
	vfree(ramdisk);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Benshushu and comment by Longtail Amethyst Eralbrunia");
MODULE_LICENSE("GPL v2");
