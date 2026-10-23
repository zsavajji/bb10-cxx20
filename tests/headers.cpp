#include "sdk-headers.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <ctime>
#include <csignal>

// Calls the SDK's <math.h> must not make ambiguous next to libc++'s.
double mixedMath(double x, int n, float f)
{
    return fmod(x, n) + pow(x, n) + atan2(f, x) + hypot(n, x) + std::fmod(f, n) + std::isnan(x)
        + fpclassify(x) + signbit(f) + std::round(f) + std::lround(x) + std::log(x) + cos(f);
}
