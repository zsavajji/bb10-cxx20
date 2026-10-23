/* BB10's <sys/cdefs.h> sets _STD_USING, so in C++ the QNX C headers declare
 * the C library in namespace std (Dinkumware's layout) and export only some
 * of it to the global namespace. libc++ expects a C library in the global
 * namespace, which it then brings into std itself: put it there. */
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
#endif

#endif
