/* BB10's <sys/cdefs.h> sets _STD_USING, so in C++ the QNX C headers declare
 * the C library in namespace std (Dinkumware's layout) and export only some
 * of it to the global namespace. libc++ expects a C library in the global
 * namespace, which it then brings into std itself: put it there.
 *
 * _GLOBAL_USING is Dinkumware's name for that layout. Twenty SDK headers
 * (<sys/stat.h>, <fcntl.h>, <stdio.h>, ...) set their include guard only
 * when it or _STD_USING is defined, so without it they are not guarded in
 * C++ and a second inclusion redefines their structs. */
#ifndef BB10_CXX20_SYS_CDEFS_H
#define BB10_CXX20_SYS_CDEFS_H

#include_next <sys/cdefs.h>

#if defined(__cplusplus)
# undef _STD_USING
# undef _CSTD
# undef _C_STD_BEGIN
# undef _C_STD_END
# define _CSTD          ::
# define _C_STD_BEGIN
# define _C_STD_END
# undef _GLOBAL_USING
# define _GLOBAL_USING 1
#endif

#endif
