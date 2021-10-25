#ifndef __RING_BUF_H
#define __RING_BUF_H

struct ring_buf
{
    int *pool;
    unsigned int size; 
    unsigned int cnt;
    unsigned int in;
    unsigned int out;
};
typedef struct ring_buf ring_buf_t;

int rb_send (ring_buf_t *rb, int val);
int rb_recv (ring_buf_t *rb, int *val);
int rb_is_empty (ring_buf_t *rb);

#endif