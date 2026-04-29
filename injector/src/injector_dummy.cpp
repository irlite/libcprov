#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct linux_dirent;
struct linux_dirent64;

static char** build_argv_from_varargs(const char* first, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    size_t argc = 1;
    while (va_arg(ap2, const char*) != nullptr) ++argc;
    va_end(ap2);

    char** argv = (char**)malloc((argc + 1) * sizeof(char*));
    if (!argv) return nullptr;
    argv[0] = (char*)first;
    for (size_t i = 1; i < argc; ++i)
        argv[i] = va_arg(ap, char*);
    argv[argc] = nullptr;
    return argv;
}

#define SAVE_ERRNO    int saved_errno = errno
#define RESTORE_ERRNO errno = saved_errno

#define RESOLVE_REAL(real_fn, sym1, sym2, failret)                        \
    do {                                                                  \
        if (!(real_fn)) {                                                 \
            (real_fn) = decltype(real_fn)(dlsym(RTLD_NEXT, (sym1)));      \
            if (!(real_fn))                                               \
                (real_fn) = decltype(real_fn)(dlsym(RTLD_NEXT, (sym2)));  \
            if (!(real_fn)) return (failret);                             \
        }                                                                 \
    } while (0)

extern "C" {

// ---------- WRITE HOOKS ----------

ssize_t write(int fd, const void* buf, size_t count) {
    static auto real_write = (ssize_t (*)(int, const void*, size_t)) nullptr;
    RESOLVE_REAL(real_write, "__libc_write", "write", (ssize_t)-1);
    return real_write(fd, buf, count);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    static auto real_fwrite = (size_t (*)(const void*, size_t, size_t, FILE*)) nullptr;
    RESOLVE_REAL(real_fwrite, "__libc_fwrite", "fwrite", (size_t)0);
    return real_fwrite(ptr, size, nmemb, stream);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    static auto real_writev = (ssize_t (*)(int, const struct iovec*, int)) nullptr;
    RESOLVE_REAL(real_writev, "__libc_writev", "writev", (ssize_t)-1);
    return real_writev(fd, iov, iovcnt);
}

ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    static auto real_pwrite = (ssize_t (*)(int, const void*, size_t, off_t)) nullptr;
    RESOLVE_REAL(real_pwrite, "__libc_pwrite", "pwrite", (ssize_t)-1);
    return real_pwrite(fd, buf, count, offset);
}

ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset) {
    static auto real_pwrite64 = (ssize_t (*)(int, const void*, size_t, off64_t)) nullptr;
    RESOLVE_REAL(real_pwrite64, "__libc_pwrite64", "pwrite64", (ssize_t)-1);
    return real_pwrite64(fd, buf, count, offset);
}

int fputs(const char* s, FILE* stream) {
    static auto real_fputs = (int (*)(const char*, FILE*)) nullptr;
    RESOLVE_REAL(real_fputs, "__libc_fputs", "fputs", -1);
    return real_fputs(s, stream);
}

int fprintf(FILE* stream, const char* fmt, ...) {
    static auto real_vfprintf = (int (*)(FILE*, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vfprintf, "__libc_vfprintf", "vfprintf", -1);
    va_list ap;
    va_start(ap, fmt);
    int ret = real_vfprintf(stream, fmt, ap);
    va_end(ap);
    return ret;
}

int vfprintf(FILE* stream, const char* fmt, va_list ap) {
    static auto real_vfprintf = (int (*)(FILE*, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vfprintf, "__libc_vfprintf", "vfprintf", -1);
    return real_vfprintf(stream, fmt, ap);
}

int dprintf(int fd, const char* fmt, ...) {
    static auto real_vdprintf = (int (*)(int, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vdprintf, "__libc_vdprintf", "vdprintf", -1);
    va_list ap;
    va_start(ap, fmt);
    int ret = real_vdprintf(fd, fmt, ap);
    va_end(ap);
    return ret;
}

int vdprintf(int fd, const char* fmt, va_list ap) {
    static auto real_vdprintf = (int (*)(int, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vdprintf, "__libc_vdprintf", "vdprintf", -1);
    return real_vdprintf(fd, fmt, ap);
}

int fputc(int c, FILE* stream) {
    static auto real_fputc = (int (*)(int, FILE*)) nullptr;
    RESOLVE_REAL(real_fputc, "__libc_fputc", "fputc", -1);
    return real_fputc(c, stream);
}

int fputs_unlocked(const char* s, FILE* stream) {
    static auto real_fputs_unlocked = (int (*)(const char*, FILE*)) nullptr;
    RESOLVE_REAL(real_fputs_unlocked, "__libc_fputs_unlocked", "fputs_unlocked", -1);
    return real_fputs_unlocked(s, stream);
}

size_t fwrite_unlocked(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    static auto real_fwrite_unlocked = (size_t (*)(const void*, size_t, size_t, FILE*)) nullptr;
    RESOLVE_REAL(real_fwrite_unlocked, "__libc_fwrite_unlocked", "fwrite_unlocked", (size_t)0);
    return real_fwrite_unlocked(ptr, size, nmemb, stream);
}

ssize_t pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset) {
    static auto real_pwritev = (ssize_t (*)(int, const struct iovec*, int, off_t)) nullptr;
    RESOLVE_REAL(real_pwritev, "__libc_pwritev", "pwritev", (ssize_t)-1);
    return real_pwritev(fd, iov, iovcnt, offset);
}

ssize_t pwritev2(int fd, const struct iovec* iov, int iovcnt, off_t offset, int flags) {
    static auto real_pwritev2 = (ssize_t (*)(int, const struct iovec*, int, off_t, int)) nullptr;
    RESOLVE_REAL(real_pwritev2, "__libc_pwritev2", "pwritev2", (ssize_t)-1);
    return real_pwritev2(fd, iov, iovcnt, offset, flags);
}

// ---------- SEND HOOKS ----------

ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count) {
    static auto real_sendfile = (ssize_t (*)(int, int, off_t*, size_t)) nullptr;
    RESOLVE_REAL(real_sendfile, "__libc_sendfile", "sendfile", (ssize_t)-1);
    return real_sendfile(out_fd, in_fd, offset, count);
}

ssize_t sendfile64(int out_fd, int in_fd, off64_t* offset, size_t count) {
    static auto real_sendfile64 = (ssize_t (*)(int, int, off64_t*, size_t)) nullptr;
    RESOLVE_REAL(real_sendfile64, "__libc_sendfile64", "sendfile64", (ssize_t)-1);
    return real_sendfile64(out_fd, in_fd, offset, count);
}

ssize_t copy_file_range(int fd_in, off64_t* off_in, int fd_out,
                        off64_t* off_out, size_t len, unsigned int flags) {
    static auto real_copy_file_range =
        (ssize_t (*)(int, off64_t*, int, off64_t*, size_t, unsigned int)) nullptr;
    RESOLVE_REAL(real_copy_file_range, "__libc_copy_file_range", "copy_file_range", (ssize_t)-1);
    return real_copy_file_range(fd_in, off_in, fd_out, off_out, len, flags);
}

ssize_t splice(int fd_in, off64_t* off_in, int fd_out, off64_t* off_out,
               size_t len, unsigned int flags) {
    static auto real_splice =
        (ssize_t (*)(int, off64_t*, int, off64_t*, size_t, unsigned int)) nullptr;
    RESOLVE_REAL(real_splice, "__libc_splice", "splice", (ssize_t)-1);
    return real_splice(fd_in, off_in, fd_out, off_out, len, flags);
}

// ---------- READ HOOKS ----------

ssize_t read(int fd, void* buf, size_t count) {
    static auto real_read = (ssize_t (*)(int, void*, size_t)) nullptr;
    RESOLVE_REAL(real_read, "__libc_read", "read", (ssize_t)-1);
    return real_read(fd, buf, count);
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    static auto real_pread = (ssize_t (*)(int, void*, size_t, off_t)) nullptr;
    RESOLVE_REAL(real_pread, "__libc_pread", "pread", (ssize_t)-1);
    return real_pread(fd, buf, count, offset);
}

ssize_t pread64(int fd, void* buf, size_t count, off64_t offset) {
    static auto real_pread64 = (ssize_t (*)(int, void*, size_t, off64_t)) nullptr;
    RESOLVE_REAL(real_pread64, "__libc_pread64", "pread64", (ssize_t)-1);
    return real_pread64(fd, buf, count, offset);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
    static auto real_readv = (ssize_t (*)(int, const struct iovec*, int)) nullptr;
    RESOLVE_REAL(real_readv, "__libc_readv", "readv", (ssize_t)-1);
    return real_readv(fd, iov, iovcnt);
}

ssize_t preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset) {
    static auto real_preadv = (ssize_t (*)(int, const struct iovec*, int, off_t)) nullptr;
    RESOLVE_REAL(real_preadv, "__libc_preadv", "preadv", (ssize_t)-1);
    return real_preadv(fd, iov, iovcnt, offset);
}

ssize_t preadv2(int fd, const struct iovec* iov, int iovcnt, off_t offset, int flags) {
    static auto real_preadv2 = (ssize_t (*)(int, const struct iovec*, int, off_t, int)) nullptr;
    RESOLVE_REAL(real_preadv2, "__libc_preadv2", "preadv2", (ssize_t)-1);
    return real_preadv2(fd, iov, iovcnt, offset, flags);
}

int getdents(unsigned int fd, struct linux_dirent* dirp, unsigned int count) {
    static auto real_getdents =
        (int (*)(unsigned int, struct linux_dirent*, unsigned int)) nullptr;
    RESOLVE_REAL(real_getdents, "__libc_getdents", "getdents", -1);
    return real_getdents(fd, dirp, count);
}

int getdents64(unsigned int fd, struct linux_dirent64* dirp, unsigned int count) {
    static auto real_getdents64 =
        (int (*)(unsigned int, struct linux_dirent64*, unsigned int)) nullptr;
    RESOLVE_REAL(real_getdents64, "__libc_getdents64", "getdents64", -1);
    return real_getdents64(fd, dirp, count);
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    static auto real_fread = (size_t (*)(void*, size_t, size_t, FILE*)) nullptr;
    RESOLVE_REAL(real_fread, "__libc_fread", "fread", (size_t)0);
    return real_fread(ptr, size, nmemb, stream);
}

char* fgets(char* s, int size, FILE* stream) {
    static auto real_fgets = (char* (*)(char*, int, FILE*)) nullptr;
    RESOLVE_REAL(real_fgets, "__libc_fgets", "fgets", nullptr);
    return real_fgets(s, size, stream);
}

int fgetc(FILE* stream) {
    static auto real_fgetc = (int (*)(FILE*)) nullptr;
    RESOLVE_REAL(real_fgetc, "__libc_fgetc", "fgetc", EOF);
    return real_fgetc(stream);
}

int getc(FILE* stream) {
    static auto real_getc = (int (*)(FILE*)) nullptr;
    RESOLVE_REAL(real_getc, "__libc_getc", "getc", EOF);
    return real_getc(stream);
}

int fscanf(FILE* stream, const char* format, ...) {
    static auto real_vfscanf = (int (*)(FILE*, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vfscanf, "__isoc99_vfscanf", "vfscanf", -1);
    va_list ap;
    va_start(ap, format);
    int ret = real_vfscanf(stream, format, ap);
    va_end(ap);
    return ret;
}

int vfscanf(FILE* stream, const char* format, va_list ap) {
    static auto real_vfscanf = (int (*)(FILE*, const char*, va_list)) nullptr;
    RESOLVE_REAL(real_vfscanf, "__isoc99_vfscanf", "vfscanf", -1);
    return real_vfscanf(stream, format, ap);
}

// ---------- EXEC HOOKS ----------

int execve(const char* pathname, char* const argv[], char* const envp[]) {
    static auto real_execve = (int (*)(const char*, char* const[], char* const[])) nullptr;
    RESOLVE_REAL(real_execve, "__libc_execve", "execve", -1);
    return real_execve(pathname, argv, envp);
}

int execveat(int dirfd, const char* pathname, char* const argv[],
             char* const envp[], int flags) {
    static auto real_execveat =
        (int (*)(int, const char*, char* const[], char* const[], int)) nullptr;
    RESOLVE_REAL(real_execveat, "__libc_execveat", "execveat", -1);
    return real_execveat(dirfd, pathname, argv, envp, flags);
}

int fexecve(int fd, char* const argv[], char* const envp[]) {
    static auto real_fexecve = (int (*)(int, char* const[], char* const[])) nullptr;
    RESOLVE_REAL(real_fexecve, "__libc_fexecve", "fexecve", -1);
    return real_fexecve(fd, argv, envp);
}

int execv(const char* path, char* const argv[]) {
    static auto real_execv = (int (*)(const char*, char* const[])) nullptr;
    RESOLVE_REAL(real_execv, "__libc_execv", "execv", -1);
    return real_execv(path, argv);
}

int execvp(const char* file, char* const argv[]) {
    static auto real_execvp = (int (*)(const char*, char* const[])) nullptr;
    RESOLVE_REAL(real_execvp, "__libc_execvp", "execvp", -1);
    return real_execvp(file, argv);
}

int execvpe(const char* file, char* const argv[], char* const envp[]) {
    static auto real_execvpe = (int (*)(const char*, char* const[], char* const[])) nullptr;
    RESOLVE_REAL(real_execvpe, "__libc_execvpe", "execvpe", -1);
    return real_execvpe(file, argv, envp);
}

int execl(const char* path, const char* arg, ...) {
    static auto real_execv = (int (*)(const char*, char* const[])) nullptr;
    RESOLVE_REAL(real_execv, "__libc_execv", "execv", -1);
    va_list ap;
    va_start(ap, arg);
    char** argv = build_argv_from_varargs(arg, ap);
    va_end(ap);
    if (!argv) { errno = ENOMEM; return -1; }
    int ret = real_execv(path, argv);
    free(argv);
    return ret;
}

int execlp(const char* file, const char* arg, ...) {
    static auto real_execvp = (int (*)(const char*, char* const[])) nullptr;
    RESOLVE_REAL(real_execvp, "__libc_execvp", "execvp", -1);
    va_list ap;
    va_start(ap, arg);
    char** argv = build_argv_from_varargs(arg, ap);
    va_end(ap);
    if (!argv) { errno = ENOMEM; return -1; }
    int ret = real_execvp(file, argv);
    free(argv);
    return ret;
}

int execle(const char* path, const char* arg, ...) {
    static auto real_execve = (int (*)(const char*, char* const[], char* const[])) nullptr;
    RESOLVE_REAL(real_execve, "__libc_execve", "execve", -1);
    va_list ap;
    va_start(ap, arg);

    va_list ap2;
    va_copy(ap2, ap);
    size_t argc = 1;
    while (va_arg(ap2, const char*) != nullptr) ++argc;
    va_end(ap2);

    char** argv = (char**)malloc((argc + 1) * sizeof(char*));
    if (!argv) { va_end(ap); errno = ENOMEM; return -1; }
    argv[0] = (char*)arg;
    for (size_t i = 1; i < argc; ++i)
        argv[i] = va_arg(ap, char*);
    argv[argc] = nullptr;

    va_arg(ap, char*);                         // consume nullptr sentinel
    char* const* envp = va_arg(ap, char* const*);
    va_end(ap);

    int ret = real_execve(path, argv, (char* const*)envp);
    free(argv);
    return ret;
}

// ---------- RENAME HOOKS ----------

int rename(const char* oldpath, const char* newpath) {
    static auto real_rename = (int (*)(const char*, const char*)) nullptr;
    RESOLVE_REAL(real_rename, "__libc_rename", "rename", -1);
    return real_rename(oldpath, newpath);
}

int renameat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath) {
    static auto real_renameat = (int (*)(int, const char*, int, const char*)) nullptr;
    RESOLVE_REAL(real_renameat, "__libc_renameat", "renameat", -1);
    return real_renameat(olddirfd, oldpath, newdirfd, newpath);
}

int renameat2(int olddirfd, const char* oldpath, int newdirfd,
              const char* newpath, unsigned int flags) {
    static auto real_renameat2 =
        (int (*)(int, const char*, int, const char*, unsigned int)) nullptr;
    RESOLVE_REAL(real_renameat2, "__libc_renameat2", "renameat2", -1);
    return real_renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
}

int unlink(const char* pathname) {
    static auto real_unlink = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_unlink, "__libc_unlink", "unlink", -1);
    return real_unlink(pathname);
}

int unlinkat(int dirfd, const char* pathname, int flags) {
    static auto real_unlinkat = (int (*)(int, const char*, int)) nullptr;
    RESOLVE_REAL(real_unlinkat, "__libc_unlinkat", "unlinkat", -1);
    return real_unlinkat(dirfd, pathname, flags);
}

int remove(const char* pathname) {
    static auto real_remove = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_remove, "__libc_remove", "remove", -1);
    return real_remove(pathname);
}

int rmdir(const char* pathname) {
    static auto real_rmdir = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_rmdir, "__libc_rmdir", "rmdir", -1);
    return real_rmdir(pathname);
}

int shm_unlink(const char* name) {
    static auto real_shm_unlink = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_shm_unlink, "__libc_shm_unlink", "shm_unlink", -1);
    return real_shm_unlink(name);
}

int mq_unlink(const char* name) {
    static auto real_mq_unlink = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_mq_unlink, "__libc_mq_unlink", "mq_unlink", -1);
    return real_mq_unlink(name);
}

int sem_unlink(const char* name) {
    static auto real_sem_unlink = (int (*)(const char*)) nullptr;
    RESOLVE_REAL(real_sem_unlink, "__libc_sem_unlink", "sem_unlink", -1);
    return real_sem_unlink(name);
}

// ---------- EXIT HOOKS ----------

void _exit(int status) {
    static void (*real__exit)(int) = nullptr;
    if (!real__exit) real__exit = (void (*)(int))dlsym(RTLD_NEXT, "_exit");
    if (real__exit) real__exit(status);
    else syscall(SYS_exit, status);
    __builtin_unreachable();
}

void _Exit(int status) {
    static void (*real__Exit)(int) = nullptr;
    if (!real__Exit) real__Exit = (void (*)(int))dlsym(RTLD_NEXT, "_Exit");
    if (real__Exit) real__Exit(status);
    else syscall(SYS_exit, status);
    __builtin_unreachable();
}

void quick_exit(int status) {
    static void (*real_quick_exit)(int) = nullptr;
    if (!real_quick_exit)
        real_quick_exit = (void (*)(int))dlsym(RTLD_NEXT, "quick_exit");
    if (real_quick_exit) real_quick_exit(status);
    else syscall(SYS_exit_group, status);
    __builtin_unreachable();
}

} // extern "C"
