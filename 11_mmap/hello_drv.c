#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/slab.h>


#define MIN(a, b) (a < b ? a : b)
static char *kernel_buf;
static int bufsize = 1024*8;
static struct class *hello_class;
static int major = 0;


static int hello_open (struct inode *node, struct file *file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_to_user(buf, kernel_buf, MIN(bufsize, size));
    return MIN(bufsize, size);
}

static ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_from_user(kernel_buf, buf, MIN(bufsize, size));
    return MIN(bufsize, size);
}

static int hello_close (struct inode *node, struct file *file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static int hello_mmap (struct file *info, struct vm_area_struct *vma)
{
    /* 获取物理地址 */
    unsigned long phy = virt_to_phys(kernel_buf);  /* phy地址单位为4k(4096) */

    /* 设置属性：cache，buffer */
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

    /* map */
    if (remap_pfn_range(vma, vma->vm_start, phy >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        printk("mmap remap_pfn_range failed\n");
        return -ENOBUFS;
    }
    return 0;
}

static struct file_operations hello = {
    .owner   = THIS_MODULE,
    .open    = hello_open,
    .read    = hello_read,
    .write   = hello_write,
    .release = hello_close,
    .mmap    = hello_mmap, 
};


static int __init hello_init (void)
{
    int err;

    /* 分配一段内存 */
    kernel_buf = kmalloc(bufsize, GFP_KERNEL);
    
    /* 把file_operations结构体告诉内核：注册驱动程序 */
    major = register_chrdev(0, "hello", &hello);

    hello_class = class_create(THIS_MODULE, "hello_class");
    err = PTR_ERR(hello_class);
    if (IS_ERR(hello_class)) {
        unregister_chrdev(major, "hello");
        return -1;
    }
    device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello");
    return 0;
}

static void __exit hello_exit (void)
{

    device_destroy(hello_class, MKDEV(major, 0));
    class_destroy(hello_class);
    unregister_chrdev(major, "hello");
    kfree(kernel_buf);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");  

