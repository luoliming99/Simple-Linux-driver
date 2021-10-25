#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>


/**
 * ./key_test <dev>
 */
int main (int argc, char **argv)
{
    int fd;
    int key_val;
    struct pollfd fds[1];
    int timeout_ms = 2000;
    int ret;

    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
    }

    fds[0].fd = fd;
    fds[0].events = POLLIN; /* 要轮训的事件（有数据可读） */

    while (1) {
        ret = poll(fds, 1, timeout_ms);
        if (ret > 0 && fds[0].revents == POLLIN) {
            /* 将数据全部读完 */
            while (1) {
                ret = read(fd, &key_val, 4);
                if (ret != 4) {
                    break;
                }
                printf("key_pin: %d, val: %d\n", key_val >> 8, key_val & 0xFF);
            } 
        } 
        
        /* 此时poll已超时，可以执行其它应用代码 */
        printf("Hello poll test\n");
    }
 
    close(fd);

    return 0;    
}