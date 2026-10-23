/* Compiled with -isystem shadow: its <unicode/uversion.h> must be found
 * before the SDK's (ICU 49). */
#include <unicode/uversion.h>
#ifndef BB10_CXX20_SHADOWED
#error "the SDK's headers come before the program's -isystem directories"
#endif
#include <stdio.h>
