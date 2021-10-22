#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>

#include "led_drv.h"


static int major;
static struct class *led_class;
static struct plat_led_operations *p_led_oprs;


/**
 * \brief 创建LED设备，下层使用
 */
void led_class_create_device (int minor)
{
	device_create(led_class, NULL, MKDEV(major, minor), NULL, "100ask_led%d", minor); /* /dev/100ask_led0,1,... */
}

/**
 * \brief 销毁LED设备，下层使用
 */
void led_class_destroy_device (int minor)
{
	device_destroy(led_class, MKDEV(major, minor));
}

/**
 * \brief LED操作注册函数，下层使用
 */
void led_operations_register (struct plat_led_operations *oprs)
{
	p_led_oprs = oprs;
}

EXPORT_SYMBOL(led_class_create_device);
EXPORT_SYMBOL(led_class_destroy_device);
EXPORT_SYMBOL(led_operations_register);


static int led_open (struct inode *node, struct file *file)
{
    int minor = iminor(node);
    
    /* 根据次设备号初始化LED */
    p_led_oprs->init(minor);
    
    return 0;
}

static ssize_t led_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    char status;
    int  ret;
    struct inode *node = file_inode(file);
    int  minor = iminor(node);

    /* copy_from_user: get data from app */
    ret = copy_from_user(&status, buf, 1);

    /* 根据此设备号和status控制LED */
    p_led_oprs->set(minor, status);

    return 1;
}

static int led_close (struct inode *node, struct file *file)
{
    int minor = iminor(node);
    
    /* 根据次设备号解初始化LED */
    p_led_oprs->deinit(minor);
    
    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_close,
};

/**
 * \brief 入口函数 
 */
static int __init led_init (void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    major = register_chrdev(0, "100ask_led", &led_fops);
    led_class = class_create(THIS_MODULE, "100ask_led_class");

    return 0;
}

/**
 * \brief 出口函数 
 */
static void __exit led_exit (void)
{
    class_destroy(led_class);
    unregister_chrdev(major, "100ask_led");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");  