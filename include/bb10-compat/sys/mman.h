/* BB10's <sys/mman.h>, plus madvise() and its MADV_* advice, as QNX 7 has
 * them: QNX 6.6 has only posix_madvise(). Implemented in libbb10-cxx20-rt.a
 * (src-rt/posix2008.c). */
#include_next <sys/mman.h>

#ifndef BB10_CXX20_SYS_MMAN_H
#define BB10_CXX20_SYS_MMAN_H
#define MADV_NORMAL     POSIX_MADV_NORMAL
#define MADV_SEQUENTIAL POSIX_MADV_SEQUENTIAL
#define MADV_RANDOM     POSIX_MADV_RANDOM
#define MADV_WILLNEED   POSIX_MADV_WILLNEED
#define MADV_DONTNEED   POSIX_MADV_DONTNEED
#ifdef __cplusplus
extern "C" {
#endif
int madvise(void *__addr, size_t __len, int __advice);
#ifdef __cplusplus
}
#endif
#endif
