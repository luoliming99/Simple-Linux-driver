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
    char status;

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

    /* read */
    read(fd, &status, 1);
    printf("key status: %d\n", status);
   
    /* close */
    close(fd);

    return 0;    
}