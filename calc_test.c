#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define CALC_BASE 0xFE000000
#define CALC_SIZE 0x1000

static void usage(const char *prog)
{
    printf("Usage: %s <a> <op> <b>\n", prog);
    printf("\n");
    printf("  a, b   unsigned 32-bit integers\n");
    printf("  op     one of: + - * /\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s 10 + 5\n", prog);
    printf("  %s 10 - 3\n", prog);
    printf("  %s 10 * 5\n", prog);
    printf("  %s 10 / 2\n", prog);
}

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        return 0;
    }

    if (argc != 4) {
        fprintf(stderr, "Error: expected 3 arguments\n\n");
        usage(argv[0]);
        return 1;
    }

    uint32_t a = (uint32_t)strtoul(argv[1], NULL, 0);
    uint32_t b = (uint32_t)strtoul(argv[3], NULL, 0);
    uint32_t op;

    if      (strcmp(argv[2], "+") == 0) op = 0;
    else if (strcmp(argv[2], "-") == 0) op = 1;
    else if (strcmp(argv[2], "*") == 0) op = 2;
    else if (strcmp(argv[2], "/") == 0) op = 3;
    else {
        fprintf(stderr, "Error: unknown operator '%s' (use + - * /)\n", argv[2]);
        return 1;
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    volatile uint32_t *calc = mmap(NULL, CALC_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, CALC_BASE);
    if (calc == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    calc[0] = a;   // A
    calc[1] = b;   // B
    calc[2] = op;  // OP
    calc[3] = 1;   // start

    int timeout = 1000000;
    while (calc[5] != 2 && --timeout > 0);

    if (timeout == 0) {
        fprintf(stderr, "TIMEOUT: status=%u\n", calc[5]);
        munmap((void *)calc, CALC_SIZE);
        close(fd);
        return 1;
    }

    printf("Result: %u\n", calc[4]);

    munmap((void *)calc, CALC_SIZE);
    close(fd);
    return 0;
}
