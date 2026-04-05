#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE_PATH "/dev/shm/ts1"
extern char** environ;

static int run_child(const char* path, const char* argv0) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(path, argv0, (char*)NULL);
        perror("execl");
        _exit(127);
    }
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return status;
}

int main() {
    FILE* fp;
    char filename[256];

    for (int i = 1; i <= 10; i++) {
        snprintf(filename, sizeof(filename), "%s/file%d.txt", BASE_PATH, i);
        fp = fopen(filename, "w");
        if (fp == NULL) {
            printf("Failed to create %s\n", filename);
            return 1;
        }
        fprintf(fp, "This is file number %d\n", i);
        fclose(fp);
    }

    snprintf(filename, sizeof filename, "%s/file1.txt", BASE_PATH);
    if (unlink(filename) != 0) perror("unlink");
    printf("10 files created in %s\n", BASE_PATH);

    char* const argv[] = {"first_exec_child1", NULL};
    execve("./test_scripts/first_exec_child1", argv, environ);
    // run_child("./test_scripts/first_exec_child2", "first_exec_child2");

    return 0;
}
