#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


/**
 * ./key_test <dev>
 */
int main (int argc, char **argv)
{
    int fd;
    int ret;
    int flags;
    int key_val;
    int i;

    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR | O_NONBLOCK);    /* 非阻塞方式打开文件 */
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
    }

    /* 非阻塞方式连续读10次数据 */
    for (i = 0; i < 10; i++) {
        if (4 == read(fd, &key_val, 4)) {
            printf("key_pin: %d, val: %d\n", key_val >> 8, key_val & 0xFF);
        } else {
            printf("for read button err\n");
        }
    }

    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);    /* flag清零非阻塞标志位 */

    /* 阻塞方式读取数据 */
    while (1) {
        if (4 == read(fd, &key_val, 4)) {
            printf("key_pin: %d, val: %d\n", key_val >> 8, key_val & 0xFF);
        } else {
            printf("while read button err\n");
        }
    }
 
    close(fd);

    return 0;    
}