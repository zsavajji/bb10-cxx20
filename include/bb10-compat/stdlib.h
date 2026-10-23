/* BB10's <stdlib.h>, plus C11's aligned_alloc(), which libbb10-cxx20-rt.a
 * provides (src-rt/c11.c). */
#include_next <stdlib.h>

#ifndef BB10_CXX20_STDLIB_H
#define BB10_CXX20_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif
void *aligned_alloc(size_t __alignment, size_t __size);
#ifdef __cplusplus
}
#endif

#endif
