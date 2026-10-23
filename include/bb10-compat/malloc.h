/* BB10's <malloc.h>, plus glibc's malloc_usable_size() (over QNX's _msize()),
 * implemented in libbb10-cxx20-rt.a (src-rt/posix2008.c). */
#include_next <malloc.h>

#ifndef BB10_CXX20_MALLOC_H
#define BB10_CXX20_MALLOC_H
#ifdef __cplusplus
extern "C" {
#endif
size_t malloc_usable_size(void *);
#ifdef __cplusplus
}
#endif
#endif
