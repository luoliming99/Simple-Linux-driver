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
    int key_val;
    int ret;

    /* 判断参数 */
    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    /* open */
    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
    }

    /* 将数据全部读完 */
    while (1) {
        ret = read(fd, &key_val, 4);
        if (ret != 4) {
            break;
        }
        printf("key_pin: %d, val: %d\n", key_val >> 8, key_val & 0xFF);
    } 
   
    /* close */
    close(fd);

    return 0;    
}