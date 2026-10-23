/* BB10's <stdlib.h>, plus C11's aligned_alloc() and mkostemp(), which its
 * libc lacks (implemented in libbb10-cxx20-rt.a, src-rt/). */
#include_next <stdlib.h>

#ifndef BB10_CXX20_STDLIB_H
#define BB10_CXX20_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif
void *aligned_alloc(size_t __alignment, size_t __size);
int mkostemp(char *__template, int __flags);
#ifdef __cplusplus
}
#endif

#endif
