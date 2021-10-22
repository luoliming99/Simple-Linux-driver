#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>


static int major;
static struct class *led_class;
static struct gpio_desc *led_gpio;


static int led_open (struct inode *node, struct file *file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* 初始化LED引脚为高电平 */
    gpiod_direction_output(led_gpio, 1);

    return 0;
}

static ssize_t led_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    char status;
    int  ret;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* copy_from_user: get data from app */
    ret = copy_from_user(&status, buf, 1);

    /* 根据status控制LED */
    gpiod_set_value(led_gpio, status);  /* 该函数设置的为逻辑值，True-亮，False-灭 */

    printk("LED %s\n", status ? "ON" : "OFF");

    return 1;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};

static int plat_led_drv_probe (struct platform_device *pdev)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* 从设备树中获取名为"led0"的引脚GPIO信息 */
    led_gpio = gpiod_get(&pdev->dev, "led0", 0);
    if (IS_ERR(led_gpio)) {
        dev_err(&pdev->dev, "Failed to get GPIO for led");
        return PTR_ERR(led_gpio);
    }

    /* 注册file_operations */
    major = register_chrdev(0, "100ask_led", &led_fops);
    led_class = class_create(THIS_MODULE, "100ask_led_class");
    if (IS_ERR(led_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "100ask_led");
        gpiod_put(led_gpio);
    }
    device_create(led_class, NULL, MKDEV(major, 0), NULL, "100ask_led0"); /* /dev/100ask_led0 */

    return 0;
}

static int plat_led_drv_remove (struct platform_device *pdev)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(led_class, MKDEV(major, 0));
    class_destroy(led_class);
    unregister_chrdev(major, "100ask_led");
    gpiod_put(led_gpio);

    return 0;
}

static const struct of_device_id ask100_leds[] = {
    {.compatible = "100ask, led_drv"},
};

static struct platform_driver plat_led_drv = {
    .probe      = plat_led_drv_probe,
    .remove     = plat_led_drv_remove,
    .driver     = {
        .name   = "100ask_led",
        .of_match_table = ask100_leds,  /* 此属性支持匹配设备树节点设备 */
    },
};

/**
 * \brief 入口函数 
 */
static int __init led_init (void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_driver_register(&plat_led_drv); 

    return 0;
}

/**
 * \brief 出口函数 
 */
static void __exit led_exit (void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    platform_driver_unregister(&plat_led_drv);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");  