#include <linux/module.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#include "key_drv.h"
#include "plat_key_dev.h"


/*  
 * Pin: GPIO5_IO01(KEY1)
 * registers:
 * IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 Address: 229_0000h base + Ch offset = 229_000Ch  
 * IOMUXC_SNVS_SW_PAD_CTL_PAD_SNVS_TAMPER1 Address: 229_0000h base + 50h offset = 229_0050h
 * GPIOx_GDIR Address: 020A_C000 + 4h offset = 20A C004h
 * GPIOx_DR Address: 020A_C000 + 0h offset = 20A C000h
 * 
 * Pin: GPIO4_IO14(KEY2)
 * registers:
 * IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B Address: 20E_0000h base + 1B0h offset = 20E_01B0h
 * IOMUXC_SW_PAD_CTL_PAD_NAND_CE1_B Address: 20E_0000h base + 43Ch offset = 20E_043Ch
 * GPIOx_GDIR Address: 020A_8000 + 4h offset = 20A 8004h
 * GPIOx_DR Address: 020A_8000 + 0h offset = 20A 8000h
 */
#define GPIO_BASE           (0x209C000)
#define GPIO_GROUP(x)       (GPIO_BASE + (x-1)*0x4000)

struct key_registers {
    volatile unsigned int *IOMUXC_MUX;   /* 设置KEY引脚与GPIO控制器相连的寄存器 */
    volatile unsigned int *IOMUXC_PAD;   /* 设置KEY引脚的工作模式（上/下拉）的寄存器 */
    volatile unsigned int *GPIOx_GDIR;   /* 设置KEY引脚输入/输出模式的寄存器 */
    volatile unsigned int *GPIOx_DR;     /* 获取KEY引脚输出高/低电平的寄存器 */ 
};
typedef struct key_registers key_registers_t;

static key_registers_t key_regs;

static int g_keypins[100];
static int g_keycnt = 0;

static int key_gpio_init (int which)
{
    /* 获取引脚的GPIO信息 */
    int group = GROUP(g_keypins[which]); 
    int pin   = PIN(g_keypins[which]);

    printk("%s %s line %d, key %d, GPIO%d_%d\n", 
            __FILE__, __FUNCTION__, __LINE__, which, group, pin);

    /* 获取KEY引脚的寄存器信息 */
    if (group == 5 && pin == 1) {
        key_regs.IOMUXC_MUX = ioremap(0x229000C, 4);    
        key_regs.IOMUXC_PAD = ioremap(0x2290050, 4);
    }
    if (group == 4 && pin == 14) {
        key_regs.IOMUXC_MUX = ioremap(0x20E01B0, 4);
        key_regs.IOMUXC_PAD = ioremap(0x20E043C, 4);
    }
    
    key_regs.GPIOx_DR   = ioremap(GPIO_GROUP(group) + 0x0, 4);
    key_regs.GPIOx_GDIR = ioremap(GPIO_GROUP(group) + 0x4, 4);

    /* 
     * enable gpio(enable default)
     * configure pin as gpio
     * configure pin as 100K Ohm Pull Up
     * configure gpio as intput 
     */
    *key_regs.IOMUXC_MUX &= ~(0xF << 0);
    *key_regs.IOMUXC_MUX |=  (0x5 << 0);
    *key_regs.IOMUXC_PAD &= ~(0x3 << 14);
    *key_regs.IOMUXC_PAD |=  (0x2 << 14);
    *key_regs.GPIOx_GDIR &= ~(0x1 << pin);

    return 0;
}

static int key_gpio_get (int which, char *status) /* 获取按键值, which-哪个按键, status:1-按下,0-弹起 */
{
    int pin = PIN(g_keypins[which]);

    printk("%s %s line %d, key %d\n", __FILE__, __FUNCTION__, __LINE__, which);

    /* 获取KEY引脚电平上传 */ 
    *status = (*key_regs.GPIOx_DR & (0x1 << pin)) >> pin;

    return 0;
}

static int key_gpio_deinit (int which)
{
    printk("%s %s line %d, key %d\n", __FILE__, __FUNCTION__, __LINE__, which);

    /* iounmap */
    iounmap(key_regs.IOMUXC_MUX);
    iounmap(key_regs.IOMUXC_PAD);
    iounmap(key_regs.GPIOx_GDIR);
    iounmap(key_regs.GPIOx_DR);
          
    return 0;
}

static struct plat_key_operations plat_key_oprs = {
    .init = key_gpio_init,
    .get  = key_gpio_get,
    .deinit = key_gpio_deinit,
};

static int plat_key_drv_probe (struct platform_device *pdev)
{
    struct resource *res;
    int i = 0;

    /* 遍历获取每个设备资源 */
    while (1) {
        res = platform_get_resource(pdev, IORESOURCE_IRQ, i++);
        if (!res)
            break;
        
        g_keypins[g_keycnt] = res->start;
        key_class_create_device(g_keycnt);  /* 依次创建KEY设备 */
        g_keycnt++;
    }
    return 0;
    
}

static int plat_key_drv_remove (struct platform_device *pdev)
{
    struct resource *res;
    int i = 0;

    while (1) {
        res = platform_get_resource(pdev, IORESOURCE_IRQ, i++);
        if (!res)
            break;
        
        g_keycnt--;
        g_keypins[g_keycnt] = -1;
        key_class_destroy_device(i);        /* 依次销毁KEY设备 */
    }
    return 0;
}

static struct platform_driver plat_key_drv = {
    .probe      = plat_key_drv_probe,
    .remove     = plat_key_drv_remove,
    .driver     = {
        .name   = "100ask_key",
    },
};

static int __init plat_key_drv_init (void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_driver_register(&plat_key_drv); 

    /* 注册KEY操作函数给上层用 */
    key_operations_register(&plat_key_oprs);
    
    return 0;
}

static void __exit plat_key_drv_exit (void)
{
    platform_driver_unregister(&plat_key_drv);
}

module_init(plat_key_drv_init);
module_exit(plat_key_drv_exit);
MODULE_LICENSE("GPL");