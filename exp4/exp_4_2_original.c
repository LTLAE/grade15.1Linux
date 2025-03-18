#include <linux/fs.h>                  // 文件系统相关
#include <linux/module.h>              // 内核模块编程接口
#include <linux/moduleparam.h>         // 模块参数
#include <linux/init.h>                // 初始化宏
#include <linux/vmalloc.h>             // 虚拟内存分配
#include <linux/blkdev.h>              // 块设备接口
#include <linux/genhd.h>               // 块设备通用接口
#include <linux/errno.h>               // 错误码定义
#include <linux/hdreg.h>               // 硬盘几何结构
#include <linux/version.h>             // 内核版本相关
 
#define MY_DEVICE_NAME "myramdisk"     // 定义设备名称
 
// 模块参数和全局变量定义
static int mybdrv_ma_no, diskmb = 256, disk_size;
static char* ramdisk;                // 指向虚拟磁盘的内存区域
static struct gendisk* my_gd;        // 存储虚拟磁盘的gendisk结构体
static spinlock_t lock;              // 保护请求队列的自旋锁
static unsigned short sector_size = 512;  // 扇区大小，默认512字节
static struct request_queue* my_request_queue;  // 块设备请求队列
 
// 允许通过模块参数传入磁盘大小，默认为256MB
module_param_named(size, diskmb, int, 0);
 
// 处理设备请求（读/写）
static void my_request(struct request_queue* q)
{
   struct request* rq;
   int size, res = 0;
   char* ptr;
   unsigned nr_sectors, sector;
 
   pr_info("start handle request\n");
 
   // 获取队列中的一个请求
   rq = blk_fetch_request(q);
   while (rq) {
       nr_sectors = blk_rq_cur_sectors(rq);  // 当前请求的扇区数
       sector = blk_rq_pos(rq);               // 请求的起始扇区
 
       ptr = ramdisk + sector * sector_size;  // 请求的数据所在的内存位置
       size = nr_sectors * sector_size;       // 请求数据的总大小
 
       // 检查是否超出虚拟磁盘的大小
       if ((ptr + size) > (ramdisk + disk_size)) {
           pr_err("end of device\n");
           goto done;
       }
 
       // 判断读写方向
       if (rq_data_dir(rq)) {  // 写操作
           pr_info("writing at sector %d, %u sectors\n", sector, nr_sectors);
           memcpy(ptr, bio_data(rq->bio), size);  // 写数据到内存
       }
       else {  // 读操作
           pr_info("reading at sector %d, %u sectors\n", sector, nr_sectors);
           memcpy(bio_data(rq->bio), ptr, size);  // 从内存读取数据
       }
   done:
       // 完成请求并继续处理队列中的下一个请求
       if (!__blk_end_request_cur(rq, res))
           rq = blk_fetch_request(q);
   }
   pr_info("handle request done\n");
}
 
// 处理ioctl请求
static int my_ioctl(struct block_device* bdev, fmode_t mode,
   unsigned int cmd, unsigned long arg)
{
   long size;
   struct hd_geometry geo;
 
   pr_info("cmd=%d\n", cmd);
 
   switch (cmd) {
   case HDIO_GETGEO:
       pr_info("HIT HDIO_GETGEO\n");
       // 伪造磁盘几何信息
       size = disk_size;
       size &= ~0x3f;  // 清除最后的6位，确保对齐
       geo.cylinders = size >> 6;  // 某种方式计算柱面数
       geo.heads = 2;              // 假设有2个磁头
       geo.sectors = 16;           // 每个磁道16个扇区
       geo.start = 4;              // 磁盘的起始扇区
 
       // 将磁盘几何信息返回给用户空间
       if (copy_to_user((void __user*)arg, &geo, sizeof(geo)))
           return -EFAULT;
 
       return 0;
   }
   pr_warn("return -ENOTTY\n");
 
   return -ENOTTY;  // 不支持的ioctl命令
}
 
// 定义块设备的操作函数
static const struct block_device_operations mybdrv_fops = {
   .owner = THIS_MODULE,
   .ioctl = my_ioctl,  // 设置ioctl操作函数
};
 
// 模块初始化函数
static int __init my_init(void)
{
   disk_size = diskmb * 1024 * 1024;  // 计算磁盘大小（字节）
   spin_lock_init(&lock);  // 初始化自旋锁
 
   // 分配虚拟磁盘内存
   ramdisk = vmalloc(disk_size);
   if (!ramdisk)
       return -ENOMEM;
 
   // 初始化请求队列
   my_request_queue = blk_init_queue(my_request, &lock);
   if (!my_request_queue) {
       vfree(ramdisk);
       return -ENOMEM;
   }
   blk_queue_logical_block_size(my_request_queue, sector_size);  // 设置块设备的扇区大小
 
   // 注册块设备
   mybdrv_ma_no = register_blkdev(0, MY_DEVICE_NAME);
   if (mybdrv_ma_no < 0) {
       pr_err("Failed registering mybdrv, returned %d\n", mybdrv_ma_no);
       vfree(ramdisk);
       return mybdrv_ma_no;
   }
 
   // 分配gendisk结构体
   my_gd = alloc_disk(16);  // 16个分区
   if (!my_gd) {
       unregister_blkdev(mybdrv_ma_no, MY_DEVICE_NAME);
       vfree(ramdisk);
       return -ENOMEM;
   }
 
   // 设置gendisk的相关参数
   my_gd->major = mybdrv_ma_no;
   my_gd->first_minor = 0;
   my_gd->fops = &mybdrv_fops;  // 设置块设备操作函数
   strcpy(my_gd->disk_name, MY_DEVICE_NAME);  // 设置设备名称
   my_gd->queue = my_request_queue;  // 设置请求队列
   set_capacity(my_gd, disk_size / sector_size);  // 设置磁盘容量
   add_disk(my_gd);  // 将设备加入系统
 
   pr_info("device successfully registered, Major No. = %d\n", mybdrv_ma_no);
   pr_info("Capacity of ram disk is: %d MB\n", diskmb);
 
   return 0;
}
 
// 模块卸载函数
static void __exit my_exit(void)
{
   del_gendisk(my_gd);  // 从系统中删除gendisk结构体
   put_disk(my_gd);  // 释放gendisk
   unregister_blkdev(mybdrv_ma_no, MY_DEVICE_NAME);  // 注销块设备
   pr_info("module successfully unloaded, Major No. = %d\n", mybdrv_ma_no);
   blk_cleanup_queue(my_request_queue);  // 清理请求队列
   vfree(ramdisk);  // 释放虚拟磁盘内存
}
 
module_init(my_init);  // 注册模块初始化函数
module_exit(my_exit);  // 注册模块卸载函数
 
MODULE_AUTHOR("Benshushu");  
MODULE_LICENSE("GPL v2");    