/* POSIX 2008 / BSD functions that libc++ uses and BB10's libc (QNX 6.6)
 * does not have. Declared by include/bb10-compat/{stdio,wchar}.h. */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
