#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILE_PATH "/tmp/testfile.txt"
#define BUF_SIZE 3

void perform_operations() {
    int fd = open(FILE_PATH, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    char read_buf[BUF_SIZE] = {0};
    char write_buf[BUF_SIZE] = "abc";
    for (int i = 0; i < 3; i++) {
        if (pread(fd, read_buf, BUF_SIZE, i * BUF_SIZE) == -1) {
            perror("Error reading file");
            close(fd);
            return;
        }
        printf("Read from offset %d: %s\n", i * BUF_SIZE, read_buf);
        if (pwrite(fd, write_buf, BUF_SIZE, i * BUF_SIZE) == -1) {
            perror("Error writing file");
            close(fd);
            return;
        }
        printf("Written to offset %d: %s\n", i * BUF_SIZE, write_buf);
    }
    if (close(fd) == -1) {
        perror("Error closing file");
    }
}

int main() {
    int fd = open(FILE_PATH, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("Error creating file");
        return EXIT_FAILURE;
    }
    close(fd);
    perform_operations();
    return 0;
}
