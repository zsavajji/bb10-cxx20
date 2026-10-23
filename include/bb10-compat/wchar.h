/* BB10's <wchar.h>, plus the POSIX 2008 functions libc++ uses that its
 * libc lacks (implemented in libbb10-cxx20-rt.a, src-rt/posix2008.c). */
#include_next <wchar.h>

/* It also defines these as macros over internal helpers. The functions exist
 * in libc; in C++, libc++ refers to them by name. */
#if defined(__cplusplus)
# undef btowc
# undef wctob
# undef wcstod
# undef wcstof
# undef wcstold
# undef wcstoul
# undef wcstoull
#endif

#ifndef BB10_CXX20_WCHAR_H
#define BB10_CXX20_WCHAR_H
#ifdef __cplusplus
extern "C" {
#endif
size_t mbsnrtowcs(wchar_t *, const char **, size_t, size_t, mbstate_t *);
size_t wcsnrtombs(char *, const wchar_t **, size_t, size_t, mbstate_t *);
size_t wcsnlen(const wchar_t *, size_t);
#ifdef __cplusplus
/* In libc, but left out of the SDK header's C++ branch. */
float wcstof(const wchar_t *, wchar_t **);
long double wcstold(const wchar_t *, wchar_t **);
#endif
#ifdef __cplusplus
}
#endif
#endif
