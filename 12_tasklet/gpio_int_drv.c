#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>


struct gpio_info {
    int pin;   
    enum of_gpio_flags flag;
    int irq;
    struct gpio_desc *gpiod;
    struct tasklet_struct tasklet;  /* 给每个中断创建一个tasklet */
};
static struct gpio_info *gpios_int_info;


static irqreturn_t gpios_int_isr (int irq, void *dev_id)
{
    struct gpio_info *gpio_int_info = dev_id;

    /* 调度中断下半部（将中断下半部函数插入软件中断处理函数链表） */
    tasklet_schedule(&gpio_int_info->tasklet);
 
    return IRQ_HANDLED;
} 

/**
 * \brief 中断下半部处理函数
 */
static void gpio_int_tasklet_func (unsigned long data)
{
    /* data ==> gpio */
    struct gpio_info *gpio_int_info = (struct gpio_info *)data;
    int val;

    val = gpiod_get_value(gpio_int_info->gpiod); /* 获取gpio引脚逻辑值 */
    printk("pin: %d, flag: %d, irq: %d, val: %d\n",  
            gpio_int_info->pin, 
            gpio_int_info->flag,
            gpio_int_info->irq,
            val);
}

static int plat_gpio_int_probe (struct platform_device *pdev)
{
    struct device_node *np;
    int gpios_cnt;
    enum of_gpio_flags flag;
    int i;
    int ret;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    np = pdev->dev.of_node;
    /* 获取节点的gpios属性中gpio的数量 */
    gpios_cnt = of_gpio_count(np);  
    if (!gpios_cnt) {
        printk("%s %s line %d, There isn't any gpio available\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }

    /* 依次取出每个gpio */
    gpios_int_info = kzalloc(sizeof(struct gpio_info) * gpios_cnt, GFP_KERNEL);
    for (i = 0; i < gpios_cnt; i++) {
        gpios_int_info[i].pin = of_get_gpio_flags(np, i, &flag);            /* 获取gpio引脚编号和flag */
        if (gpios_int_info[i].pin < 0) {
            printk("%s %s line %d, of_get_gpio_flags err\n", __FILE__, __FUNCTION__, __LINE__);
            return -1;
        }
        gpios_int_info[i].flag  = flag & OF_GPIO_ACTIVE_LOW;                /* 保存gpio引脚的flag */
        gpios_int_info[i].irq   = gpio_to_irq(gpios_int_info[i].pin);       /* 获取gpio引脚的irq中断号 */
        gpios_int_info[i].gpiod = gpio_to_desc(gpios_int_info[i].pin);      /* 转换gpio引脚为gpiod，以便使用gpiod新函数 */

        /* 
         * 初始化tasklet 
         * 最后一个参数将作为gpio_int_tasklet_func()函数的输入参数传进去。
         */
        tasklet_init(&gpios_int_info[i].tasklet, gpio_int_tasklet_func, (unsigned long)&gpios_int_info[i]);
    }

    /* 依次请求irq */
    for (i = 0; i < gpios_cnt; i++) {
        ret = request_irq(gpios_int_info[i].irq, 
                          gpios_int_isr, 
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                          "100ask_gpio_int_irq", 
                          &gpios_int_info[i]);
    }
    return 0;
}

static int plat_gpio_int_remove (struct platform_device *pdev)
{
    struct device_node *np;
    int gpios_cnt;
    int i;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    np = pdev->dev.of_node;
    /* 依次释放irq */
    gpios_cnt = of_gpio_count(np);
    for (i = 0; i < gpios_cnt; i++) {
        free_irq(gpios_int_info[i].irq, &gpios_int_info[i]);
        tasklet_kill(&gpios_int_info[i].tasklet);   /* kill tasklet */
    }
    return 0;
}

static const struct of_device_id ask100_gpios_int[] = {
    {.compatible = "100ask, gpio_int"},
};

static struct platform_driver plat_gpio_int_drv = {
    .probe      = plat_gpio_int_probe,
    .remove     = plat_gpio_int_remove,
    .driver     = {
        .name   = "100ask_gpio_int",
        .of_match_table = ask100_gpios_int,  /* 此属性支持匹配设备树节点设备 */
    },
};

/**
 * \brief 入口函数 
 */
static int __init gpio_int_init (void)
{
    int err;
    
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = platform_driver_register(&plat_gpio_int_drv); 

    return 0;
}

/**
 * \brief 出口函数 
 */
static void __exit gpio_int_exit (void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    platform_driver_unregister(&plat_gpio_int_drv);
}

module_init(gpio_int_init);
module_exit(gpio_int_exit);
MODULE_LICENSE("GPL");  