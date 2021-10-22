#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>

#include "key_drv.h"


static int major;
static struct class *key_class;
static struct plat_key_operations *p_key_oprs;


/**
 * \brief 创建key设备，下层使用
 */
void key_class_create_device (int minor)
{
	device_create(key_class, NULL, MKDEV(major, minor), NULL, "100ask_key%d", minor); /* /dev/100ask_key0,1,... */
}

/**
 * \brief 销毁key设备，下层使用
 */
void key_class_destroy_device (int minor)
{
	device_destroy(key_class, MKDEV(major, minor));
}

/**
 * \brief key操作注册函数，下层使用
 */
void key_operations_register (struct plat_key_operations *oprs)
{
	p_key_oprs = oprs;
}

EXPORT_SYMBOL(key_class_create_device);
EXPORT_SYMBOL(key_class_destroy_device);
EXPORT_SYMBOL(key_operations_register);


static int key_open (struct inode *node, struct file *file)
{
    int minor = iminor(node);
    
    /* 根据次设备号初始化key */
    p_key_oprs->init(minor);
    
    return 0;
}

static ssize_t key_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    char status;
    int  ret;
    struct inode *node = file_inode(file);
    int  minor = iminor(node);

    /* 调用下层提供的函数获取按键电平 */
    p_key_oprs->get(minor, &status);

    /* 将按键电平发送给应用程序 */
    ret = copy_to_user(buf, &status, 1);

    return 1;
}

static int key_close (struct inode *node, struct file *file)
{
    int minor = iminor(node);
    
    /* 根据次设备号解初始化key */
    p_key_oprs->deinit(minor);
    
    return 0;
}

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .release = key_close,
};

/**
 * \brief 入口函数 
 */
static int __init my_key_init (void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    major = register_chrdev(0, "100ask_key", &key_fops);
    key_class = class_create(THIS_MODULE, "100ask_key_class");

    return 0;
}

/**
 * \brief 出口函数 
 */
static void __exit my_key_exit (void)
{
    class_destroy(key_class);
    unregister_chrdev(major, "100ask_key");
}

module_init(my_key_init);
module_exit(my_key_exit);
MODULE_LICENSE("GPL");  