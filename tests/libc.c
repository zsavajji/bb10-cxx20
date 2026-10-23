/* The functions libbb10-cxx20-rt.a adds to BB10's libc. */
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int failures;

static void check(const char *name, int ok)
{
    printf("%-28s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok)
        ++failures;
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0); /* a crash keeps what was printed */
    const char text[] = "BlackBerry 10 on QNX";
    check("memmem", memmem(text, sizeof text - 1, "10", 2) == text + 11 && !memmem(text, 4, "Berry", 5));
    check("memrchr", memrchr(text, 'B', sizeof text - 1) == text + 5);
    check("strchrnul", *strchrnul(text, 'z') == '\0' && strchrnul(text, 'Q') == text + 17);

    char *buffer = NULL;
    check("asprintf", asprintf(&buffer, "%s %d", "OS", 10) == 5 && !strcmp(buffer, "OS 10"));
    free(buffer);
    void *block = malloc(100);
    check("malloc_usable_size", malloc_usable_size(block) >= 100 && malloc_usable_size(NULL) == 0);
    free(block);
    void *aligned = aligned_alloc(4096, 4096);
    check("aligned_alloc", aligned && ((size_t)aligned & 4095) == 0);
    free(aligned);

    const char *tmpdir = getenv("TMPDIR");
    char template[512];
    snprintf(template, sizeof template, "%s/bb10-cxx20-XXXXXX", tmpdir ? tmpdir : "/tmp");
    int fd = mkostemp(template, O_CLOEXEC);
    check("mkostemp", fd != -1 && (fcntl(fd, F_GETFD) & FD_CLOEXEC));
    FILE *file = fd != -1 ? fdopen(fd, "w+") : NULL;
    char *line = NULL;
    size_t size = 0;
    ssize_t first = -1, second = -1, end = 0;
    if (file) {
        fputs("first line\nsecond", file);
        rewind(file);
        first = getline(&line, &size, file);
        int ok = first == 11 && !strcmp(line, "first line\n");
        second = getline(&line, &size, file);
        ok = ok && second == 6 && !strcmp(line, "second");
        end = getline(&line, &size, file);
        check("getline", ok && end == -1);
        fclose(file);
        unlink(template);
    } else {
        check("getline", 0);
    }
    free(line);

    int fds[2];
    check("pipe2", pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0 && (fcntl(fds[0], F_GETFD) & FD_CLOEXEC)
            && (fcntl(fds[1], F_GETFL) & O_NONBLOCK));
    int copy = dup3(fds[0], 100, O_CLOEXEC);
    check("dup3", copy == 100 && (fcntl(copy, F_GETFD) & FD_CLOEXEC) && dup3(5, 5, 0) == -1 && errno == EINVAL);
    close(copy);
    close(fds[0]);
    close(fds[1]);

    size_t length = 1 << 16;
    void *pages = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    check("madvise", pages != MAP_FAILED && madvise(pages, length, MADV_WILLNEED) == 0
            && madvise(pages, length, MADV_DONTNEED) == 0);
    munmap(pages, length);

    printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
