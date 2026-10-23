// -*- C++ -*-
//===----------------------------------------------------------------------===//
// BB10's libc (QNX 6.6) has no locale_t and none of the *_l functions, like
// Fuchsia's: only the "C" locale, through libc++'s own fallbacks.
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___LOCALE_LOCALE_BASE_API_QNX_H
#define _LIBCPP___LOCALE_LOCALE_BASE_API_QNX_H

#include <cstddef> // NULL, which __nop_locale_mgmt.h uses
#include <__support/xlocale/__nop_locale_mgmt.h>
#include <clocale>

// QNX's LC_* are already bit masks (_CATMASK), not indices to shift.
#undef LC_COLLATE_MASK
#undef LC_CTYPE_MASK
#undef LC_MONETARY_MASK
#undef LC_NUMERIC_MASK
#undef LC_TIME_MASK
#undef LC_MESSAGES_MASK
#undef LC_ALL_MASK
#define LC_COLLATE_MASK LC_COLLATE
#define LC_CTYPE_MASK LC_CTYPE
#define LC_MONETARY_MASK LC_MONETARY
#define LC_NUMERIC_MASK LC_NUMERIC
#define LC_TIME_MASK LC_TIME
#define LC_MESSAGES_MASK LC_MESSAGES
#define LC_ALL_MASK LC_ALL

#include <__support/xlocale/__posix_l_fallback.h>
#include <__support/xlocale/__strtonum_fallback.h>
#include <cstdlib>
#include <cwchar>

#endif // _LIBCPP___LOCALE_LOCALE_BASE_API_QNX_H
