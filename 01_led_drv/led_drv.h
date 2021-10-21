#ifndef __LED_DRV_H__
#define __LED_DRV_H__

struct plat_led_operations {
    int (*init)     (int which);                /* 初始化LED，which表示哪一个LED */
    int (*set)      (int which, char status);   /* 设置LED状态，which表示哪一个LED，status表示LED状态（1：亮，0：灭） */   
    int (*deinit)   (int which);                /* 解初始化LED，which表示哪一个LED */
};

void led_class_create_device (int minor);
void led_class_destroy_device (int minor);
void led_operations_register (struct plat_led_operations *oprs);

#endif