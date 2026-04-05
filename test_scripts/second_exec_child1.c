#include <stdio.h>

#define BASE_PATH "/dev/shm/ts1"

int main() {
    FILE* fp;
    char filename[256];

    snprintf(filename, sizeof(filename), "%s/file%d.txt", BASE_PATH, 2);
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Failed to create %s\n", filename);
        return 1;
    }
    fprintf(fp, "This is file number %d\n", 11);
    fclose(fp);
}
