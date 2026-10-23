/* BB10 has no <langinfo.h> (nor nl_langinfo()). libc++'s locale.cpp
 * includes it without needing anything from it on this platform. */
#ifndef BB10_CXX20_LANGINFO_H
#define BB10_CXX20_LANGINFO_H
typedef int nl_item;
#endif
