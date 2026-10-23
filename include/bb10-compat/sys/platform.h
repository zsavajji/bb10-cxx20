/* BB10's <sys/platform.h> declares, in C++, `typedef bool _Bool;`. clang's
 * <stdbool.h> defines _Bool as a macro for bool in C++, which turns that
 * into `typedef bool bool;` when <stdbool.h> comes first. */
#if defined(__cplusplus) && defined(_Bool)
#pragma push_macro("_Bool")
#undef _Bool
#include_next <sys/platform.h>
#pragma pop_macro("_Bool")
#else
#include_next <sys/platform.h>
#endif
