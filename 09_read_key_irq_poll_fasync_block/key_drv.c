#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/poll.h>
#include <linux/fcntl.h>

#include "ring_buf.h" 


struct gpio_info {
    int pin;   
    enum of_gpio_flags flag;
    int irq;
    struct gpio_desc *gpiod;
};
static struct gpio_info *gpios_int_info;

static int major;
static struct class *key_class;

static DECLARE_WAIT_QUEUE_HEAD(key_wait_q);

static struct fasync_struct *key_fasync_s;

/** 
 * 按键值环形缓存区，高8位存储引脚编号，低8位存储引脚逻辑值 
 */
static int key_val_pool[10];
static ring_buf_t key_val_rb = {
    .pool = key_val_pool,
    .size = sizeof(key_val_pool) / sizeof(int),
    .cnt  = 0,
    .in   = 0,
    .out  = 0,
};


static ssize_t key_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    int ret, val;

    /* 当环形缓存区为空并且应用程序使用非阻塞方式来读取，就立即返回 */
    if (rb_is_empty(&key_val_rb) && (file->f_flags & O_NONBLOCK)) {
        return -EAGAIN;
    }

    /* 休眠等待线程被唤醒 */
    wait_event_interruptible(key_wait_q, !rb_is_empty(&key_val_rb));    

    ret = rb_recv(&key_val_rb, &val);
    if (ret != 0) {     /* 环形缓存区为空 */
        return 0;
    }
    ret = copy_to_user(buf, &val, 4);

    return 4;
}

static unsigned int key_poll (struct file *file, poll_table *wait)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    
    poll_wait(file, &key_wait_q, wait); /* 在超时时间内休眠等待线程被唤醒 */
    return rb_is_empty(&key_val_rb) ? 0 : POLLIN | POLLRDNORM;
}

static int key_fasync (int fd, struct file *file, int on)
{
    if (fasync_helper(fd, file, on, &key_fasync_s) >= 0) {
        return 0;
    } else {
        return -EIO;
    }
}

static struct file_operations key_fops = {
    .owner  = THIS_MODULE,
    .read   = key_read,
    .poll   = key_poll,
    .fasync = key_fasync,
};


static irqreturn_t gpios_int_isr (int irq, void *dev_id)
{
    struct gpio_info *gpio_int_info = dev_id;
    int val;

    val = gpiod_get_value(gpio_int_info->gpiod); /* 获取gpio引脚逻辑值 */
    val = (gpio_int_info->pin << 8) | val;
    rb_send(&key_val_rb, val);
    wake_up_interruptible(&key_wait_q);         /* 唤醒等待队列中的线程 */
    kill_fasync(&key_fasync_s, SIGIO, POLLIN);  /* 满足POLLIN条件时就发送SIGIO信号给应用程序 */

    printk("pin: %d, flag: %d, irq: %d, val: 0x%x\n",  
            gpio_int_info->pin, 
            gpio_int_info->flag,
            gpio_int_info->irq,
            val);

    return IRQ_HANDLED;
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
    }

    /* 依次请求irq */
    for (i = 0; i < gpios_cnt; i++) {
        ret = request_irq(gpios_int_info[i].irq, 
                          gpios_int_isr, 
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                          "100ask_gpio_int_irq", 
                          &gpios_int_info[i]);
    }

    /* 注册file_operations */
    major = register_chrdev(0, "100ask_key", &key_fops);
    key_class = class_create(THIS_MODULE, "100ask_key_class");
    device_create(key_class, NULL, MKDEV(major, 0), NULL, "100ask_key0"); /* /dev/100ask_key0 */

    return 0;
}

static int plat_gpio_int_remove (struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpios_cnt;
    int i;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(key_class, MKDEV(major, 0));
    class_destroy(key_class);
    unregister_chrdev(major, "100ask_key");

    /* 依次释放irq */
    gpios_cnt = of_gpio_count(np);
    for (i = 0; i < gpios_cnt; i++) {
        free_irq(gpios_int_info[i].irq, &gpios_int_info[i]);
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