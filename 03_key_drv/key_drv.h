#ifndef __KEY_DRV_H__
#define __KEY_DRV_H__

struct plat_key_operations {
    int (*init)     (int which);                /* 初始化按键，which表示哪一个按键 */
    int (*get)      (int which, char *status);  /* 获取按键状态，which表示哪一个按键，status表示按键状态（1：按下，0：弹起） */   
    int (*deinit)   (int which);                /* 解初始化按键，which表示哪一个按键 */
};

void key_class_create_device (int minor);
void key_class_destroy_device (int minor);
void key_operations_register (struct plat_key_operations *oprs);

#endif