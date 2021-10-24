#include <linux/module.h>
#include "ring_buf.h"


int rb_send (ring_buf_t *rb, int val)
{
    if (rb == (void *)0) {
        printk("The ring buffer pointer is empty\n");
        return -1;
    }

    if (rb->cnt == rb->size) {
        printk("The ring buffer is full\n");
        return -1;
    }

    rb->pool[rb->in] = val;
    ++ rb->in;
    if (rb->in >= rb->size) {
        rb->in = 0;
    }

    if (rb->cnt < rb->size) {
        rb->cnt ++;
    } else {
        printk("Ring buffer overflow\n");
        return -1;
    }
    return 0;
}

int rb_recv (ring_buf_t *rb, int *val)
{
    if (rb == (void *)0) {
        printk("The ring buffer pointer is empty\n");
        return -1;
    }

    if (rb->cnt == 0) {
        printk("The ring buffer is empty\n");
        return -1;
    }

    *val = rb->pool[rb->out];
    ++ rb->out;
    if (rb->out >= rb->size) {
        rb->out = 0;
    }

    if (rb->cnt > 0) {
        rb->cnt --;
    }
    return 0;
}

int rb_is_empty (ring_buf_t *rb)
{
    return (!rb->cnt);
}
