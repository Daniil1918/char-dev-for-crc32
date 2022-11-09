#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#define DEVICE "/dev/crc"

typedef struct test_data_s {
    char *str_first_part;
    char *str_second_part;
    char *str_full;
}  test_data_t;

uint32_t get_crc(int fd, char *data);

int main(void) {
    int fd;
    uint32_t crc_parts, crc_full;
    test_data_t test_data = {
        .str_first_part = "Hello, ",
        .str_second_part = "World!",
        .str_full = "Hello, World!"
    };

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Error opening");
        exit(EXIT_FAILURE);
    }

    get_crc(fd, test_data.str_first_part);
    crc_parts = get_crc(fd, test_data.str_second_part);

    if (close(fd) < 0) {
        perror("Error closing");
        exit(EXIT_FAILURE);
    }

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Error opening");
        exit(EXIT_FAILURE);
    }

    crc_full = get_crc(fd, test_data.str_full);

    if (close(fd) < 0) {
        perror("Error closing");
        exit(EXIT_FAILURE);
    }

    assert(crc_parts == crc_full);

    exit(EXIT_SUCCESS);
}

uint32_t get_crc(int fd, char *data)
{
    size_t count;
    ssize_t nr;
    uint32_t crc;

    count = strlen(data);
    nr = write(fd, data, count);
    if (nr < 0 || nr != count) {
        perror("Error writing");
        exit(EXIT_FAILURE);
    }

    nr = read(fd, &crc, 4);
    if (nr != 4) {
        perror("Error reading");
        exit(EXIT_FAILURE);
    }

    return crc;
}