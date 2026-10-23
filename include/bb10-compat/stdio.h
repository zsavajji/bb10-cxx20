/* BB10's <stdio.h>, plus asprintf() and vasprintf(), which libc++ uses, and
 * POSIX 2008's getline() and getdelim(): functions its libc lacks
 * (implemented in libbb10-cxx20-rt.a, src-rt/posix2008.c). */
#include_next <stdio.h>

#ifndef BB10_CXX20_STDIO_H
#define BB10_CXX20_STDIO_H
#include <stdarg.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
int asprintf(char **, const char *, ...) __attribute__((__format__(__printf__, 2, 3)));
int vasprintf(char **, const char *, va_list) __attribute__((__format__(__printf__, 2, 0)));
ssize_t getdelim(char **, size_t *, int, FILE *);
ssize_t getline(char **, size_t *, FILE *);
#ifdef __cplusplus
}
#endif
#endif
