#include <linux/kernel.h>  	
#include <linux/module.h>  
#include <linux/fs.h>  
#include <linux/string.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/vmalloc.h>  
#include <linux/slab.h>  
#include <linux/sched.h>  
#include <asm/io.h>  
#include <linux/mman.h>  

#define MAP_PAGE_COUNT 10  
#define MAPLEN (PAGE_SIZE*MAP_PAGE_COUNT)  
// define MAP_DEV_MAJOR 240
#define MAP_DEV_MAJOR 260
#define MAP_DEV_NAME "mapnopage"

extern struct mm_struct init_mm;  
void map_vopen(struct vm_area_struct *vma);
void map_vclose(struct vm_area_struct *vma);  
/*device mmap */  
static int mapdrv_mmap(struct file *file, struct vm_area_struct *vma);  
static int mapdrv_open(struct inode *inode, struct file *file); 
/* vm area nopage */  
int map_fault(struct vm_fault *vmf);  
  
static struct file_operations mapdrvo_fops = {  
    .owner = THIS_MODULE,  
    .mmap = mapdrv_mmap,  
    .open = mapdrv_open,
}; 

static struct vm_operations_struct map_vm_ops = {
    .open = map_vopen,
    .close = map_vclose,
    // .fault = map_fault,
    .fault = (vm_fault_t (*)(struct vm_fault *))map_fault,
};
   
 
static char *vmalloc_area = NULL;  
 
// entry ->
static int __init mapdrv_init(void)  
{  
    int result;
    unsigned long virt_addr;
    int i = 1;
    // register a character device
    // major num: 240, name: mapnopage, pointer to file_operations
    result=register_chrdev(MAP_DEV_MAJOR,MAP_DEV_NAME,&mapdrvo_fops);
    // if register failed, return error code
    if(result<0){
        return result;
    }
    // allocate an area in kernel virturl space and transfrom its address to unsigned long
    vmalloc_area=vmalloc(MAPLEN);
    virt_addr = (unsigned long)vmalloc_area;
    // I don't know why virt_addr is assigned twice
    // for each page, set it reserved and write an unique string into it
    for(virt_addr = (unsigned long)vmalloc_area; virt_addr < (unsigned long)vmalloc_area + MAPLEN; virt_addr += PAGE_SIZE)
    {
        // sprintf(virtual address, test, page number)
        SetPageReserved(vmalloc_to_page((void *)virt_addr));   
            sprintf((char *)virt_addr, "test %d",i++);             
    }
    /* printk("vmalloc_area at 0x%lx (phys 0x%lx)\n",(unsigned long)vmalloc_area,(unsigned long)vmalloc_to_pfn((void *)vmalloc_area) << PAGE_SHIFT);  */
    printk("vmalloc area apply complate!");
    return 0;
}  

// exit <-
static void __exit mapdrv_exit(void)  
{  
    unsigned long virt_addr;  
    /* unreserve all pages */  
    for(virt_addr = (unsigned long)vmalloc_area; virt_addr < (    unsigned long)vmalloc_area + MAPLEN; virt_addr += PAGE_SIZE) 
    {  
        ClearPageReserved(vmalloc_to_page((void *)virt_addr));  
    }  
    /* and free the two areas */  
    if (vmalloc_area)
        vfree(vmalloc_area);  
    unregister_chrdev(MAP_DEV_MAJOR,MAP_DEV_NAME);
  
}  
  

  
static int mapdrv_mmap(struct file *file, struct vm_area_struct *vma)  
{  
    // vma: memory area, vm_pgoff: offset, shift left 12 bits aka 4KB, size of a page
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;  
    // end - start = size
    unsigned long size = vma->vm_end - vma->vm_start;  
    // as you can see, oversize check here
    if (size > MAPLEN) {  
        printk("size too big\n");  
        return -ENXIO;  
    }  
    /*  only support shared mappings. */ 
    // only shared mappings are supported 
    if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {  
        printk("writeable mappings must be shared, rejecting\n");  
        return -EINVAL;  
    }  
    /* do not want to have this area swapped out, lock it */  
    // lock it to prevent this area being swapped out
    vma->vm_flags |= VM_LOCKONFAULT;  
    // offset = 0 aka mapping starts at the beginning of the memory area
    if (offset == 0) {  
        // ensure correct operation of managing the area
        vma->vm_ops = &map_vm_ops;   
    } else {  
        // offset out of range
        printk("offset out of range\n");  
        return -ENXIO;  
    }  
    return 0;  
}
static int mapdrv_open(struct inode *inoe, struct file *file)
{
    // print the process name and pid
    printk("process: %s (%d)\n", current->comm, current->pid);
    return 0;
}  
  
/* open handler for vm area */  
void map_vopen(struct vm_area_struct *vma)  
{  
    printk("mapping vma is opened..\n");
}  
  
/* close handler form vm area */  
void map_vclose(struct vm_area_struct *vma)  
{  
    printk("mapping vma is closed..\n");
}  
  
/* page fault handler */ 
int map_fault(struct vm_fault *vmf)  
{  
	struct page *page;
	void *page_ptr;
    // address of the memory area
    unsigned long offset, virt_start, pfn_start;	
    offset = vmf->address-vmf->vma->vm_start;
    virt_start = (unsigned long)vmalloc_area + (unsigned long)(vmf->pgoff << PAGE_SHIFT);
    pfn_start = (unsigned long)vmalloc_to_pfn((void *)virt_start);

	printk("\n");    
	/*printk("%-25s %d\n","7)PAGE_SHIFT",PAGE_SHIFT);*/
	page_ptr=NULL;
    // if init fail or memory allocation fail, return SIGBUS
	if((vmf->vma==NULL)||(vmalloc_area==NULL)){
		printk("return VM_FAULT_SIGBUS!\n");
		return VM_FAULT_SIGBUS;
	}
    // if offset out of range, return SIGBUS
	if(offset >=MAPLEN){
		printk("return VM_FAULT_SIGBUS!");
		return VM_FAULT_SIGBUS;
	}
    // get the page pointer
	page_ptr=vmalloc_area + offset;
    // get the page struct
	page=vmalloc_to_page(page_ptr);
    // get the page
	get_page(page);	
    // set the page to *vmf
	vmf->page=page; 
        printk("%s: map 0x%lx (0x%016lx) to 0x%lx , size: 0x%lx, page:%ld \n", __func__, virt_start, pfn_start << PAGE_SHIFT, vmf->address,PAGE_SIZE,vmf->pgoff);

	return 0;
}

module_init(mapdrv_init);  
module_exit(mapdrv_exit);  

MODULE_LICENSE("GPL?");
MODULE_AUTHOR("Not Longtail Amethyst Eralbrunia");