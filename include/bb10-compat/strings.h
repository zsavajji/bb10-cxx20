/* BB10's <strings.h>, in C++, defines bcmp(), bzero() and others inline over
 * memcmp() and memset(), and includes <string.h> for them -- but in C++ that
 * finds libc++'s <string.h>, which may be the very header being processed,
 * so nothing is declared yet. Bring in the C library's <string.h> first. */
#ifndef BB10_CXX20_STRINGS_H
#define BB10_CXX20_STRINGS_H
#if defined(__cplusplus)
# include_next <string.h>
#endif
#include_next <strings.h>
#endif
