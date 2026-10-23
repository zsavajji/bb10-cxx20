/* POSIX 2008, BSD and GNU functions that libc++ and ported libraries use
 * and BB10's libc (QNX 6.6) does not have. Declared by the wrappers in
 * include/bb10-compat/. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wchar.h>

int vasprintf(char **out, const char *fmt, va_list ap)
{
    va_list copy;
    va_copy(copy, ap);
    int n = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    *out = NULL;
    if (n < 0)
        return -1;
    char *buf = malloc((size_t)n + 1);
    if (!buf)
        return -1;
    int written = vsnprintf(buf, (size_t)n + 1, fmt, ap);
    if (written < 0) {
        free(buf);
        return -1;
    }
    *out = buf;
    return written;
}

int asprintf(char **out, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vasprintf(out, fmt, ap);
    va_end(ap);
    return n;
}

size_t wcsnlen(const wchar_t *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n])
        ++n;
    return n;
}

/* Converts at most nms bytes of *src, writing at most len wide characters
 * to dst (or counting them when dst is NULL). */
size_t mbsnrtowcs(wchar_t *dst, const char **src, size_t nms, size_t len, mbstate_t *ps)
{
    static mbstate_t internal;
    if (!ps)
        ps = &internal;
    const char *s = *src;
    size_t count = 0;
    while (nms > 0 && (!dst || count < len)) {
        wchar_t wc;
        size_t r = mbrtowc(&wc, s, nms, ps);
        if (r == (size_t)-1) {
            if (dst)
                *src = s;
            return (size_t)-1;
        }
        if (r == (size_t)-2) /* incomplete within nms: stop, state kept */
            break;
        if (r == 0) { /* the terminating null */
            if (dst) {
                dst[count] = L'\0';
                *src = NULL;
            }
            return count;
        }
        if (dst)
            dst[count] = wc;
        s += r;
        nms -= r;
        ++count;
    }
    if (dst)
        *src = s;
    return count;
}

/* Converts at most nwc wide characters of *src, writing at most len bytes to
 * dst (or counting them when dst is NULL). */
size_t wcsnrtombs(char *dst, const wchar_t **src, size_t nwc, size_t len, mbstate_t *ps)
{
    static mbstate_t internal;
    if (!ps)
        ps = &internal;
    const wchar_t *s = *src;
    size_t count = 0;
    char buf[MB_LEN_MAX];
    while (nwc > 0) {
        mbstate_t saved = *ps;
        size_t r = wcrtomb(buf, *s, ps);
        if (r == (size_t)-1) {
            if (dst)
                *src = s;
            return (size_t)-1;
        }
        if (dst) {
            if (count + r > len) { /* does not fit: leave it for next time */
                *ps = saved;
                break;
            }
            memcpy(dst + count, buf, r);
        }
        if (*s == L'\0') {
            if (dst)
                *src = NULL;
            return count + r - 1; /* not counting the null */
        }
        count += r;
        ++s;
        --nwc;
    }
    if (dst)
        *src = s;
    return count;
}

void *memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
    const unsigned char *h = haystack;
    if (!needle_len)
        return (void *)h;
    for (; haystack_len >= needle_len; ++h, --haystack_len) {
        if (*h == *(const unsigned char *)needle && !memcmp(h, needle, needle_len))
            return (void *)h;
    }
    return NULL;
}

void *memrchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s + n;
    while (n--) {
        if (*--p == (unsigned char)c)
            return (void *)p;
    }
    return NULL;
}

char *strchrnul(const char *s, int c)
{
    while (*s && *s != (char)c)
        ++s;
    return (char *)s;
}

ssize_t getdelim(char **line, size_t *size, int delim, FILE *stream)
{
    size_t length = 0;
    int c;
    if (!line || !size) {
        errno = EINVAL;
        return -1;
    }
    if (!*line)
        *size = 0;
    while ((c = getc(stream)) != EOF) {
        if (length + 2 > *size) {
            size_t grown = *size ? *size * 2 : 128;
            char *bigger = realloc(*line, grown);
            if (!bigger)
                return -1;
            *line = bigger;
            *size = grown;
        }
        (*line)[length++] = (char)c;
        if (c == delim)
            break;
    }
    if (!length)
        return -1; /* end of file or error, nothing read */
    (*line)[length] = '\0';
    return (ssize_t)length;
}

ssize_t getline(char **line, size_t *size, FILE *stream)
{
    return getdelim(line, size, '\n', stream);
}

/* The descriptor flags, set after the descriptor exists: not atomic with a
 * fork() in another thread, which QNX 6.6 offers no way to be. */
static int set_fd_flags(int fd, int flags)
{
    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        return -1;
    if (flags & ~O_CLOEXEC) {
        int status = fcntl(fd, F_GETFL);
        if (status == -1 || fcntl(fd, F_SETFL, status | (flags & ~O_CLOEXEC)) == -1)
            return -1;
    }
    return 0;
}

int mkostemp(char *template, int flags)
{
    int fd = mkstemp(template);
    if (fd != -1 && set_fd_flags(fd, flags) == -1) {
        int error = errno;
        close(fd);
        unlink(template);
        errno = error;
        return -1;
    }
    return fd;
}

int pipe2(int fds[2], int flags)
{
    if (flags & ~(O_CLOEXEC | O_NONBLOCK)) {
        errno = EINVAL;
        return -1;
    }
    if (pipe(fds) == -1)
        return -1;
    if (set_fd_flags(fds[0], flags) == -1 || set_fd_flags(fds[1], flags) == -1) {
        int error = errno;
        close(fds[0]);
        close(fds[1]);
        errno = error;
        return -1;
    }
    return 0;
}

int dup3(int oldfd, int newfd, int flags)
{
    if (oldfd == newfd || (flags & ~O_CLOEXEC)) {
        errno = EINVAL;
        return -1;
    }
    if (dup2(oldfd, newfd) == -1)
        return -1;
    if (set_fd_flags(newfd, flags) == -1) {
        int error = errno;
        close(newfd);
        errno = error;
        return -1;
    }
    return newfd;
}

/* QNX 7's madvise() over QNX 6.6's posix_madvise(), which returns the error
 * instead of setting errno. */
int madvise(void *addr, size_t len, int advice)
{
    int error = posix_madvise(addr, len, advice);
    if (error) {
        errno = error;
        return -1;
    }
    return 0;
}

/* glibc's name for the usable size of a heap block; QNX calls it _msize()
 * (in libc, not in its headers). */
extern size_t _msize(void *);

size_t malloc_usable_size(void *p)
{
    return p ? _msize(p) : 0;
}
