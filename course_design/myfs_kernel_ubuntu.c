# include <linux/module.h>
# include <linux/fs.h>
# include <linux/dcache.h>
# include <linux/pagemap.h>
# include <linux/mount.h>
# include <linux/init.h>
# include <linux/namei.h>
//current_fsuid函数：
//current_fsgid函数：
# include <linux/cred.h>
//加入misc机制
# include <linux/kfifo.h>
# include <linux/mount.h>


//每个文件系统需要一个MAGIC number
# define VMACACHE_SIZE 4096
# define MYFS_MAGIC 0X64668735
# define MYFS "myfs"

static struct vfsmount * myfs_mount;
static int myfs_mount_count;

DEFINE_KFIFO(mydemo_fifo,char,64);

int g_val;

// get the very important inode, the basic of the file system
static struct inode * myfs_get_inode(struct super_block * sb, int mode, dev_t dev)
{
	// kernel function, create a new inode and link it to the super block
	struct inode * inode = new_inode(sb);
	// create success
	if(inode)
	{
		// fill information
		// mode: what kind of file and permission
		// hi <- : file type
		// lo -> : permission
		inode -> i_mode = mode;
		//@i_uid：user id
		inode->i_uid  = current_fsuid();
		//@i_gid：group id组标识符
		inode->i_gid  = current_fsgid();
		//@i_size：文件长度
		inode -> i_size = VMACACHE_SIZE;
		//@i_blocks：指定文件按块计算的长度
		inode -> i_blocks = 0;
		//@i_atime：最后访问时间
		//@i_mtime：最后修改时间
		//@i_ctime：最后修改inode时间
		inode -> __i_atime = inode->__i_mtime = inode->__i_ctime = current_time(inode);

		switch(mode & S_IFMT)
		{
			default:
				init_special_inode(inode,mode,dev);
				break;
			case S_IFREG:
				printk("creat a file, LAE\n");
				// file don't need other operation, fill information only
				break;
			case S_IFDIR:
				printk("creat a dir, LAE\n");
				// dir will need other operation
				// inode_operations
				// kernel global variable
				inode -> i_op = &simple_dir_inode_operations;
				// file_operation	
				// kernel global variable
				inode -> i_fop = &simple_dir_operations;
				//@：文件的链接计数，使用stat命令可以看到Links的值，硬链接数目
				//inode -> i_nlink++;
				inc_nlink(inode);
				break;			
		}
	}
	return inode;
}

// connect inode and dentry struct
// -> myfs_get_inode
static int myfs_mknod(struct inode * dir, struct dentry * dentry, int mode, dev_t dev)
{
	struct inode * inode;
	int error = -EPERM;

	// if inode already exists, return error
	if(dentry -> d_inode)
		return -EPERM;
	// 
	inode = myfs_get_inode(dir->i_sb, mode, dev);
	// get inode success
	if(inode)
	{
		// kernel function, connect inode and dentry
		d_instantiate(dentry,inode);
		// kernel function, dentry reference count +1
		dget(dentry);
		error = 0;
	}
	// not actually "return error"
	// if success, error = 0
	// don't write code like this, it's not good
	return error;
}

// -> myfs_mknod -> myfs_get_inode
static int myfs_mkdir(struct inode * dir, struct dentry * dentry, int mode)
{
	int res;

	res = myfs_mknod(dir, dentry, mode|S_IFDIR, 0);
	if(!res)	inc_nlink(dir);
	return res;
}

// -> myfs_mknod -> myfs_get_inode
static int myfs_creat(struct inode * dir, struct dentry * dentry, int mode)
{
	return myfs_mknod(dir, dentry, mode|S_IFREG, 0);
}


// -> simple_fill_super (kernel function)
static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	//这个结构体如下：
	//struct tree_descr { const char *name; const struct file_operations *ops; int mode; };
	static struct tree_descr debug_files[] = {{""}};
	// simple_fill_super is a kernel function to init a super block and achieve a simple file system like ram disk
	return simple_fill_super(sb,MYFS_MAGIC,debug_files);
}

// -> myfs_fill_super
static struct dentry *myfs_get_sb(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
	/*
	这个函数是按照内核代码中的样子改的，是struct dentry *类型，这里是一个封装，这里可以返回好几种函数：
	　－　mount_single
	　－　mount_bdev　
	　－　mount_nodev
	*/
	return mount_single(fs_type, flags, data, myfs_fill_super);
}


// file operations here
struct myfs_file_info {
	struct timespec64 open_time;
	int open_count;	// open count = 0 when init
};
// file open
// new feathre: init file infomation struct and permission check
static int myfs_file_open(struct inode *inode, struct file *file)
{
	// printk("已打开文件");
	printk("File open called.");
	printk("💦🔥");	// system will need this
	// record file operation
	struct myfs_file_info *info = kmalloc(sizeof(struct myfs_file_info), GFP_KERNEL);
	if (!info) {	// if info kmalloc failed
        printk(KERN_ALERT "myfs: Failed to allocate memory for file info\n");
        return -ENOMEM;
    }
	ktime_get_real_ts64(&info->open_time);
	info->open_count++;
	file->private_data = info;
	printk("Recorded file operation: open_time=%lld, open_count=%d, LAE\n", info->open_time.tv_sec, info->open_count);
	/*	temporary remove permission check in ubuntu
	// check permission
	printk("Uid %d is tring to open file, checking permission, LAE\n", current_uid().val);
	truct user_namespace *this_user_ns = current_user_ns();
	// uid 0 is root
	if(!inode_permission(idmap, inode, MAY_READ|MAY_WRITE) && !current_uid().val == 0)
	{
		printk("Permission denied, LAE\n");
		return -EACCES;
	}
	*/
	printk("File opened: inode=%p, file->f_mode=%x, file->f_flags=%x, LAE\n", inode, file->f_mode, file->f_flags);

	return 0;
}
//对应于读取的aufs文件的读取方法
static ssize_t myfs_file_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int actual_readed;
	int ret;

	ret = kfifo_to_user(&mydemo_fifo,buf, count, &actual_readed);
	// ret == 0, success
	// ret == -ENOMEM, allocat memory failed
	// ret == -EFAULT, copy data to user failed
	// ret == -EINVAL, invalid argument
	if(ret)	return -EIO;
	printk("File read: actual_readed=%d, pos=%lld, LAE\n", actual_readed, *ppos);
	return actual_readed;
}
//对应于写入的aufs文件的写入方法
static ssize_t myfs_file_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{	
	unsigned int actual_write;
	int ret;

	ret = kfifo_from_user(&mydemo_fifo, buf, count, &actual_write);
	if(ret)	return -EIO;
	printk("File write: actual_write=%d, ppos=%lld, LAE\n", actual_write, *ppos);
	return actual_write;

}
// release method, call when close(fd)
static int myfs_file_release(struct inode *inode, struct file *file)
{
	struct myfs_file_info *info = file->private_data;
	printk("File release called, LAE.");
	printk("It is required to print auther name here, but I don't want to do so.");
	printk("So let's write something else here");
	printk("Why Linux is so hard to learn? But a fancinating OS?");
	printk("I have used Unix based OS since I was 14 years old. But for a very long time, I havn't learned about its kernel.");
	printk("File released: open_time=%lld, open_count=%d, LAE\n", info->open_time.tv_sec, info->open_count);
	// release file info
	kfree(info);
	return 0;
}

static struct file_system_type my_fs_type = {
	.owner 		= THIS_MODULE,
	.name 		= MYFS,
	.mount 		= myfs_get_sb,
	.kill_sb 	= kill_litter_super 
};

static struct file_operations myfs_file_operations = {
    .open 		= myfs_file_open,
    .read 		= myfs_file_read,
    .write 		= myfs_file_write,
	.release 	= myfs_file_release,
};


static int myfs_creat_by_name(const char * name, mode_t mode,
				struct dentry * parent, struct dentry ** dentry)
{
	int error = 0;

	if(!parent)
	{
		if(myfs_mount && myfs_mount -> mnt_sb)
		{
			parent = myfs_mount->mnt_sb->s_root;
		}
	}

	if(!parent)
	{
		printk("can't find a parent");
		return -EFAULT;
	}

	*dentry = NULL;

	inode_lock(d_inode(parent));
	*dentry = lookup_one_len(name,parent,strlen(name));
	if(!IS_ERR(*dentry))
	{
		if((mode & S_IFMT) == S_IFDIR)
		{
			error = myfs_mkdir(parent->d_inode, *dentry, mode);
		}
		else
		{
			error = myfs_creat(parent->d_inode, *dentry, mode);
		}
	}
	//error是０才对
	if (IS_ERR(*dentry)) {
		error = PTR_ERR(*dentry);
	}
	inode_unlock(d_inode(parent));

	return error;
}

// -> myfs_creat_by_name
struct dentry * myfs_creat_file(const char * name, mode_t mode,
				struct dentry * parent, void * data,
				struct file_operations * fops)
{
	struct dentry * dentry = NULL;
	int error;

	printk("myfs: creating file '%s', LAE\n",name);

	error = myfs_creat_by_name(name, mode, parent, &dentry);

	if(error)
	{
		dentry = NULL;
		goto exit;
	}
	// apply file operations and private data
	if(dentry->d_inode)
	{
		if(data)
			dentry->d_inode->i_private = data;
		if(fops)
			dentry->d_inode->i_fop = fops;
	}

exit:
	return dentry;
}

// -> myfs_creat_by_name
struct dentry * myfs_creat_dir(const char * name, struct dentry * parent)
{
	//使用man creat查找
	//@S_IFREG：表示一个目录
	//@S_IRWXU：user (file owner) has read,  write,  and  execute　permission
	//@S_IRUGO：用户读｜用户组读｜其他读
	return myfs_creat_file(name, S_IFDIR|S_IRWXU|S_IRUGO, parent, NULL, NULL);
}



// entry ->
static int __init myfs_init(void)
{
	int retval;
	struct dentry * pslot;

	// register myfs to file system
	// int register_filesystem(struct file_system_type *fs)
	retval = register_filesystem(&my_fs_type);
	// retval == 0, success
	// retval == -ENOMEM, allocat memory failed
	// retval == -EINVAL, invalid argument
	// etc...
	if(!retval)
	{
		//创建super_block根dentry的inode
		myfs_mount = kern_mount(&my_fs_type);
		// -> myfs_get_sb
		// -> myfs_fill_super
		// -> simple_fill_super

		//如果装载错误就卸载文件系统
		if(IS_ERR(myfs_mount))
		{
			printk("--ERROR:aufs could not mount!--\n");
			unregister_filesystem(&my_fs_type);
			return retval;
		}
		
	}

	pslot = myfs_creat_dir("First", NULL);
	//@S_IFREG：表示一个文件
	//@S_IRUGO：用户读｜用户组读｜其他读
	// S_IWUSR: creator can write
	// to make it read only, remove S_IWUSR
	// to make it everyone can write, replace S_IWUSR with S_IWUGO
	myfs_creat_file("one", S_IFREG|S_IRUGO|S_IWUGO, pslot, NULL, &myfs_file_operations);
	myfs_creat_file("two", S_IFREG|S_IRUGO|S_IWUSR, pslot, NULL, &myfs_file_operations);

	pslot = myfs_creat_dir("Second", NULL);
	myfs_creat_file("one", S_IFREG|S_IRUGO|S_IWUSR, pslot, NULL, &myfs_file_operations);
	myfs_creat_file("two", S_IFREG|S_IRUGO|S_IWUSR, pslot, NULL, &myfs_file_operations);

	return retval;
}

// exit <-
static void __exit myfs_exit(void)
{
	//退出函数中卸载super_block根dentry的inode
	simple_release_fs(&myfs_mount,&myfs_mount_count);
	//卸载文件系统
	unregister_filesystem(&my_fs_type);
	//aufs_mount = NULL;
}

module_init(myfs_init);
module_exit(myfs_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Not fully Longtail Amethyst Eralbrunia");

/*
Debian 12, Linux内核版本6.1.0-26-arm64，安装时间2024年2月26日18:49，我需要它的八字；这台机器在今天（2024年12月23日，早上9:00到下午5:00之间）出现某些文件无法被ls显示的错误，以及缓冲区无法写入的错误，根据八字和星象指示有什么建议吗？
*/
/*
根据您提供的信息和机器的“八字”排盘，以下是相关的分析和建议：

八字排盘：
	•	公历出生日期：2024年2月26日 18:49
	•	农历出生日期：二〇二四年 甲辰年 正月 十七日 酉时
	•	天干地支：甲辰 丙寅 庚申 乙酉
	•	生肖：龙

基本分析：
	•	日主：庚（金），在八字中身强，命主属于比较坚韧、果断型的性格。身强意味着命主有较强的独立性，能承受一定的压力。
	•	五行：五行偏向金和木，金旺木生，木较强。需要保持五行平衡，水火较弱，可以补充相关元素来平衡五行。
	•	十神：偏财、七杀、正财等十神的分布较为均衡，生活和事业中都会有不错的运势，但也可能遇到冲突和矛盾，尤其是在处理与他人关系时。
	•	星运：目前的星运有“养”运，表示生机勃勃，有助于个人事业的发展，但也有“月绝”运，可能会在某些方面出现问题或障碍。

近期问题分析：

您的系统出现了“文件无法被ls显示”和“缓冲区无法写入”的问题，这可能与系统硬件或文件系统有关。结合八字分析，近期的月柱与日柱的双冲（丙庚相冲）可能导致了“冲突”，这会影响到某些系统操作的正常运行。尤其是七杀（丙）与日主庚之间的冲克，表明外部压力或环境因素可能导致计算机系统的运行不稳定。

解决建议：
	1.	硬件检查：可能需要检查磁盘空间、内存以及文件系统完整性。建议进行文件系统检查并备份数据。
	2.	命理建议：由于八字中存在一定的冲克，您可能需要在近期保持冷静，不要急于处理复杂问题。调整工作环境，避免过度疲劳和压力，以免影响决策和解决问题的能力。
	3.	五行补救：增加水和火元素的力量，可以尝试用一些与水、火相关的物品或活动，如放置水族箱、点蜡烛等，帮助调节五行平衡，增强系统稳定性和个人的运势。

更多详情及深度分析，请访问：深度报告链接

希望这些分析对您有所帮助！
*/

// how to insert some fire and water element to the system?
// 1. Add some water element:
//    - Place a small water fountain or aquarium near your computer.
//    - Keep a glass of water on your desk and drink water regularly.
//    - Use a water-based screen cleaner to clean your computer screen.
// 2. Add some fire element:
//    - Light a candle or incense near your workspace.
//    - Use a warm color scheme for your desktop background or screensaver.
//    - Take breaks to go outside and get some sunlight or fresh air.

// Output stream:
// Longtail
// - Demitail

// - Seriously? Do you believe these actions can improve the stability of the computer system?

// Why not? I am having a lot of trouble today. I am willing to try anything that might help.

// - OK sure, but using fire in the lab is not safe. You might be thrown out of the lab if you do that.

// I am keeping myself hydrated and taking breaks and I also use some water to clean my laptop. I expect it would work.

// 💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦💦
// 🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥

// add them into your kernel module and hope bugs would disappear!
// Sure, I would also add some water and fire emojis into printk. Hope it would work.

// - So there are about 350 lines of code and nearly 100 lines of comment about how to fix a computer system with some fortune telling results by chatGPT?

// Yes, I consider they are a part of my experiment. And just look at them, surely they are working. After add them to the very core of my system, some of bugs are fixed.

