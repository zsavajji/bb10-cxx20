/* BB10's <string.h>, plus memmem(), memrchr() and strchrnul(), which its
 * libc lacks (implemented in libbb10-cxx20-rt.a, src-rt/posix2008.c). */
#include_next <string.h>

#ifndef BB10_CXX20_STRING_H
#define BB10_CXX20_STRING_H
#ifdef __cplusplus
extern "C" {
#endif
void *memmem(const void *, size_t, const void *, size_t);
void *memrchr(const void *, int, size_t);
char *strchrnul(const char *, int);
#ifdef __cplusplus
}
#endif
#endif
