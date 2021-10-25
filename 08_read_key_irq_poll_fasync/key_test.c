#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>


int fd;


static void sig_func (int sig)
{
    int key_val;
    int ret;
    
    /* 将数据全部读完 */
    while (1) {
        ret = read(fd, &key_val, 4);
        if (ret != 4) {
            break;
        }
        printf("key_pin: %d, val: %d\n", key_val >> 8, key_val & 0xFF);
    } 
}

/**
 * ./key_test <dev>
 */
int main (int argc, char **argv)
{
    int ret;
    int flags;

    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    signal(SIGIO, sig_func);

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
    }

    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | FASYNC);

    while (1) {
        printf("Hello fasync test\n");
        sleep(2);
    }
 
    close(fd);

    return 0;    
}