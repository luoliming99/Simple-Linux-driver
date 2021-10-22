#ifndef __PLAT_KEY_DEV_H__
#define __PLAT_KEY_DEV_H__

#define GROUP_PIN(g, p)     ((g << 16) | (p))
#define GROUP(x)            (x >> 16)
#define PIN(x)              (x & 0xFFFF)

#endif