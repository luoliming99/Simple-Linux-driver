#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

/**
 * ./led_test /dev/myled ON
 * ./led_test /dev/myled OFF
 */
int main (int argc, char **argv)
{
    int fd;
    char status;

    /* 判断参数 */
    if (argc != 3) {
        printf("Usage: %s <dev> ON\n", argv[0]);
        printf("       %s <dev> OFF\n", argv[0]);
        return -1;
    }

    /* open */
    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
    }

    /* write */
    if (0 == strcmp(argv[2], "ON")) {
        status = 1;
    } else if (0 == strcmp(argv[2], "OFF")) {
        status = 0;
    }
    write(fd, &status, 1);
   
    /* close */
    close(fd);

    return 0;    
}