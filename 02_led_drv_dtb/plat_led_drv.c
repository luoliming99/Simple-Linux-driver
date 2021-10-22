#include <linux/module.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include "led_drv.h"
#include "plat_led_dev.h"


/*  
 * Pin: GPIO5_IO03
 * registers:
 * IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 Address: 229_0000h base + 14h offset = 229_0014h 
 * GPIOx_GDIR Address: 020A_C000 + 4h offset = 20A C004h
 * GPIOx_DR Address: 020A_C000 + 0h offset = 20A C000h
 */
#define GPIO_BASE           (0x209C000)
#define GPIO_GROUP(x)       (GPIO_BASE + (x-1)*0x4000)

struct led_registers {
    volatile unsigned int *IOMUXC;       /* 设置LED引脚与GPIO控制器相连的寄存器 */
    volatile unsigned int *GPIOx_GDIR;   /* 设置LED引脚输入/输出模式的寄存器 */
    volatile unsigned int *GPIOx_DR;     /* 设置LED引脚输出高/低电平的寄存器 */ 
};
typedef struct led_registers led_registers_t;

static led_registers_t led_regs;

static int g_ledpins[100];
static int g_ledcnt = 0;

static int led_gpio_init (int which)
{
    /* 获取引脚的GPIO信息 */
    int group = GROUP(g_ledpins[which]); 
    int pin   = PIN(g_ledpins[which]);

    printk("%s %s line %d, led %d, GPIO%d_%d\n", 
            __FILE__, __FUNCTION__, __LINE__, which, group, pin);

    /* 获取LED引脚的寄存器信息 */
    if (group == 5 && pin == 3) {
        led_regs.IOMUXC = ioremap(0x2290014, 4);
    }
    led_regs.GPIOx_DR   = ioremap(GPIO_GROUP(group) + 0x0, 4);
    led_regs.GPIOx_GDIR = ioremap(GPIO_GROUP(group) + 0x4, 4);

    /* 
     * enable gpio(enable default)
     * configure pin as gpio
     * configure gpio as output 
     */
    *led_regs.IOMUXC     &= ~(0xF << 0);
    *led_regs.IOMUXC     |=  (0x5 << 0);
    *led_regs.GPIOx_GDIR |=  (0x1 << pin);
    *led_regs.GPIOx_DR   |=  (0x1 << pin); /* 初始为高电平 */ 

    return 0;
}

static int led_gpio_set (int which, char status) /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
{
    int pin = PIN(g_ledpins[which]);

    printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__, which, status ? "ON" : "OFF");

    /* 根据status设置LED引脚电平 */ 
    if (status) {
        *led_regs.GPIOx_DR &= ~(0x1 << pin);    /* 低电平，LED亮 */
    } else {
        *led_regs.GPIOx_DR |=  (0x1 << pin);    /* 高电平，LED灭 */
    }

    return 0;
}

static int led_gpio_deinit (int which)
{
    printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);

    /* iounmap */
    iounmap(led_regs.IOMUXC);
    iounmap(led_regs.GPIOx_GDIR);
    iounmap(led_regs.GPIOx_DR);
          
    return 0;
}

static struct plat_led_operations plat_led_oprs = {
    .init = led_gpio_init,
    .set  = led_gpio_set,
    .deinit = led_gpio_deinit,
};

static int plat_led_drv_probe (struct platform_device *pdev)
{
    struct device_node *np;
    int led_pin;

    np = pdev->dev.of_node;
    /*
     * 这个platform_driver支持的platform_device，可能来自设备树，
     * 也可能不是来自设备树，所以这里要判断一下。
     */
    if (!np) {
        return -1;
    }

    /* 获取LED引脚信息 */   
    of_property_read_u32(np, "pin", &led_pin);

    g_ledpins[g_ledcnt] = led_pin;
    led_class_create_device(g_ledcnt); /* 创建LED设备 */
    g_ledcnt++;

    return 0;
}

static int plat_led_drv_remove (struct platform_device *pdev)
{
    struct device_node *np;
    int led_pin;
    int i = 0;

    np = pdev->dev.of_node;
    /*
     * 这个platform_driver支持的platform_device，可能来自设备树，
     * 也可能不是来自设备树，所以这里要判断一下。
     */
    if (!np) 
        return -1;

    /* 获取LED引脚信息 */
    of_property_read_u32(np, "pin", &led_pin);

    /* 销毁LED设备 */
    for (i = 0; i < g_ledcnt; i++) {
        if (g_ledpins[i] == led_pin) {
            led_class_destroy_device(i); 
            g_ledpins[i] = -1;
            break;
        }
    }

    /* 如果所有pin都为-1，就清零cnt */
    for (i = 0; i < g_ledcnt; i++) {
        if (g_ledpins[i] != -1) {
            break;
        }
    }
    if (i == g_ledcnt) {
        g_ledcnt = 0;
    }
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

static int __init plat_led_drv_init (void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_driver_register(&plat_led_drv); 

    /* 注册LED操作函数给上层用 */
    led_operations_register(&plat_led_oprs);
    
    return 0;
}

static void __exit plat_led_drv_exit (void)
{
    platform_driver_unregister(&plat_led_drv);
}

module_init(plat_led_drv_init);
module_exit(plat_led_drv_exit);
MODULE_LICENSE("GPL");