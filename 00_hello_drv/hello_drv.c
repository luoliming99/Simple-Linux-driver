#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>

#define MIN(a, b) (a < b ? a : b)
static char kernel_buf[1024];
static struct class *hello_class;

/* 1. 确定主设备号 */
static int major = 0;

/* 3. 实现对应的open/read/write等函数，填入file_operations结构体 */
static int hello_open (struct inode *node, struct file *file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_to_user(buf, kernel_buf, MIN(1024, size));
    return MIN(1024, size);
}

static ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_from_user(kernel_buf, buf, MIN(1024, size));
    return MIN(1024, size);
}

static int hello_close (struct inode *node, struct file *file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

/* 2. 定义自己的file_operations结构体 */
static struct file_operations hello = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .read = hello_read,
    .write = hello_write,
    .release = hello_close,
};


/* 4. 谁来注册驱动程序？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数 */
static int __init hello_init (void)
{
    int err;

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

/* 5. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数 */
static void __exit hello_exit (void)
{
    device_destroy(hello_class, MKDEV(major, 0));
    class_destroy(hello_class);
    unregister_chrdev(major, "hello");
}

/* 6. 其它完善：提供设备信息，自动创建设备节点 */
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");  

