/* BB10's <sys/siginfo.h> typedefs time_t and clock_t in namespace std (it
 * uses _STD_BEGIN) when it is the first header to see __TIME_T/__CLOCK_T,
 * then uses them as ::time_t. Declare them in the global namespace first and
 * consume the markers, as the other QNX C headers do, so it skips its own. */
#if defined(__cplusplus)
# include <sys/platform.h>
# if defined(__TIME_T)
typedef __TIME_T time_t;
#  undef __TIME_T
# endif
# if defined(__CLOCK_T)
typedef __CLOCK_T clock_t;
#  undef __CLOCK_T
# endif
#endif

#include_next <sys/siginfo.h>
