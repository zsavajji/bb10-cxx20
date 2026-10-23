/* BB10's <unistd.h>, plus pipe2() and dup3(), which its libc lacks
 * (implemented in libbb10-cxx20-rt.a, src-rt/posix2008.c). They set their
 * flags with fcntl() after the fact, so not atomically with fork(). */
#include_next <unistd.h>

#ifndef BB10_CXX20_UNISTD_H
#define BB10_CXX20_UNISTD_H
#ifdef __cplusplus
extern "C" {
#endif
int pipe2(int __fds[2], int __flags);
int dup3(int __oldfd, int __newfd, int __flags);
#ifdef __cplusplus
}
#endif
#endif
