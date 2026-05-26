#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE_PATH "/dev/shm/ts1"

int main() {
    FILE* fp;
    char filename[256];
    char buffer[100];
    for (int i = 2; i <= 10; i += 2) {
        snprintf(filename, sizeof(filename), "%s/file%d.txt", BASE_PATH, i);
        fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Could not open %s\n", filename);
            continue;
        }
        printf("Contents of %s:\n", filename);
        while (fgets(buffer, sizeof(buffer), fp)) {
            printf("%s", buffer);
        }
        printf("\n");
        fclose(fp);
    }

    snprintf(filename, sizeof(filename), "%s/existing_file2.txt", BASE_PATH);
    fp = fopen(filename, "a");
    if (fp != NULL) {
        fprintf(fp, "Accessed by program\n");
        fclose(fp);
    }

    fp = fopen(filename, "r");
    if (fp != NULL) {
        char buffer[100];
        while (fgets(buffer, sizeof(buffer), fp)) {
            printf("%s", buffer);
        }
        fclose(fp);
    }

    snprintf(filename, sizeof(filename), "%s/file2.txt", BASE_PATH);
    unlink(filename);
    printf("Deleted %s\n", filename);
    snprintf(filename, sizeof(filename), "%s/file4.txt", BASE_PATH);
    fp = fopen(filename, "a");
    fprintf(fp, "Appended by second_exec (write to file4)\n");
    fclose(fp);
    unlink(filename);
    printf("Deleted %s\n", filename);
    snprintf(filename, sizeof(filename), "%s/file6.txt", BASE_PATH);
    fp = fopen(filename, "a");
    fprintf(fp, "Appended by second_exec (write to file6)\n");
    fclose(fp);
    printf("Wrote to %s\n", filename);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        char* argv[] = {(char*)"./test_scripts/second_exec_child1",
                        (char*)"example_param", NULL};
        execv(argv[0], argv);
        perror("execv");
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    return 0;
}
