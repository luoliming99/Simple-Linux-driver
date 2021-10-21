#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#include "plat_led_dev.h"


static struct resource led_resource[] = {
    {
        .start = GROUP_PIN(5, 3),
        .flags = IORESOURCE_IRQ,
        .name = "100ask_led_pin",
    },
    {
        .start = GROUP_PIN(5, 3),
        .flags = IORESOURCE_IRQ,
        .name = "100ask_led_pin",
    },
};

static struct platform_device plat_led_dev = {
        .name = "100ask_led",
        .num_resources = ARRAY_SIZE(led_resource),
        .resource = led_resource,
};


static int __init plat_led_dev_init(void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_device_register(&plat_led_dev);   
    
    return 0;
}

static void __exit plat_led_dev_exit(void)
{
    platform_device_unregister(&plat_led_dev);
}

module_init(plat_led_dev_init);
module_exit(plat_led_dev_exit);
MODULE_LICENSE("GPL");