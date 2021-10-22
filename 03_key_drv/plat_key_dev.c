#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#include "plat_key_dev.h"


static struct resource key_resource[] = {
    {
        .start = GROUP_PIN(5, 1),
        .flags = IORESOURCE_IRQ,
        .name = "100ask_key_pin",
    },
    {
        .start = GROUP_PIN(4, 14),
        .flags = IORESOURCE_IRQ,
        .name = "100ask_key_pin",
    },
};

static struct platform_device plat_key_dev = {
        .name = "100ask_key",
        .num_resources = ARRAY_SIZE(key_resource),
        .resource = key_resource,
};


static int __init plat_key_dev_init(void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_device_register(&plat_key_dev);   
    
    return 0;
}

static void __exit plat_key_dev_exit(void)
{
    platform_device_unregister(&plat_key_dev);
}

module_init(plat_key_dev_init);
module_exit(plat_key_dev_exit);
MODULE_LICENSE("GPL");