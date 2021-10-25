#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

/**
 * ./hello_test
 */
int main (int argc, char **argv)
{
    int fd;
    char *vm_addr;
    char buf[1024];

    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return -1;
    }

    fd = open("/dev/hello", O_RDWR);
    if (fd == -1) {
        printf("can not open file /dev/hello\n");
        return -1;
    }

    /* mmap */
    vm_addr = mmap(NULL, 1024*8, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (vm_addr == (void *)-1) {
        printf("can not map file /dev/hello\n");
        return -1;
    }
    printf("mmap address: 0x%x\n", vm_addr);

    /* write */
    strcpy(vm_addr, "I am the data written to the memory");

    /* read & compare */
    read(fd, buf, 1024);
    if (strcmp(vm_addr, buf) == 0) {
        printf("strcmp ok\n");
    } else {
        printf("strcmp err\n");
    }

    while (1) {
        sleep(10);  /* cat /proc/pid/maps 查看此进程的虚拟地址空间 */
    }

    /* munmap */
    munmap(vm_addr, 1024*8);
    close(fd);

    return 0;
}