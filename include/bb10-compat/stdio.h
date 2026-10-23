/* BB10's <stdio.h>, plus asprintf() and vasprintf(), which libc++ uses and
 * its libc lacks (implemented in libbb10-cxx20-rt.a, src-rt/posix2008.c). */
#include_next <stdio.h>

#ifndef BB10_CXX20_STDIO_H
#define BB10_CXX20_STDIO_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
int asprintf(char **, const char *, ...) __attribute__((__format__(__printf__, 2, 3)));
int vasprintf(char **, const char *, va_list) __attribute__((__format__(__printf__, 2, 0)));
#ifdef __cplusplus
}
#endif
#endif
