/* BB10's <math.h>, outside its own C++ branch (which libc++ replaces),
 * defines cos(), sin(), log() and their variants as macros over internal
 * helpers. The real functions are declared too; libc++ declares its
 * overloads under these names, so the macros go in C++. */
#include_next <math.h>

#if defined(__cplusplus)
# undef cos
# undef cosf
# undef cosl
# undef cosh
# undef coshf
# undef coshl
# undef log
# undef logf
# undef logl
# undef log10
# undef log10f
# undef log10l
# undef log2
# undef log2f
# undef log2l
# undef sin
# undef sinf
# undef sinl
# undef sinh
# undef sinhf
# undef sinhl
#endif
