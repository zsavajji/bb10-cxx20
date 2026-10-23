#!/bin/sh
# Builds the tests into OUT (default out/tests) with the toolchain at
# BB10_CXX20 (default /opt/bb10-cxx20), and checks that every symbol they
# import is in BB10's libc/libm or in libc++_shared.so. Then copy OUT to the
# phone and run run-tests.sh there.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-out/tests}
TC=${BB10_CXX20:-/opt/bb10-cxx20}
CXX="$TC/bin/bb10-clang++"
CC="$TC/bin/bb10-clang"
B=/opt/bb10-toolchain/bin/arm-unknown-nto-qnx6.5.0eabi-
LIB=${QNX_TARGET:-/opt/bb10-sdk/target/qnx6}/armle-v7/lib
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)

# The SDK's headers, in C and in C++ (compile only).
"$CC" -fsyntax-only "$HERE/headers.c"
"$CXX" -fsyntax-only "$HERE/headers.cpp"
# A program's -isystem directories come before the SDK's headers.
"$CC" -fsyntax-only -isystem "$HERE/shadow" "$HERE/include-order.c"
"$CXX" -fsyntax-only -isystem "$HERE/shadow" -x c++ "$HERE/include-order.c"

"$CXX" -O2 "$HERE/cxx20.cpp" -o "$OUT/cxx20"
"$CXX" -O2 -shared-libc++ "$HERE/cxx20.cpp" -o "$OUT/cxx20-shared"
"$CXX" -O2 -shared-libc++ -shared "$HERE/shape.cpp" -o "$OUT/libshape.so" -Wl,-soname,libshape.so
"$CXX" -O2 -shared-libc++ "$HERE/modules.cpp" -o "$OUT/modules" -L"$OUT" -lshape
"$CXX" -O2 -shared "$HERE/plugin.cpp" -o "$OUT/plugin.so"
"$CC" -O2 "$HERE/dlopen.c" -o "$OUT/dlopen"
"$CC" -O2 -Wall -Werror "$HERE/libc.c" -o "$OUT/libc"
# A file under its SONAME: symlinks do not survive copying to the phone.
cp "$TC/lib/libc++_shared.so.1" "$OUT/"

rm -rf "$OUT/cmake-build"
cmake -S "$HERE/cmake" -B "$OUT/cmake-build" -DCMAKE_TOOLCHAIN_FILE="$TC/share/bb10-cxx20.cmake" \
    -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$OUT/cmake-build" >/dev/null
cp "$OUT/cmake-build/cmake-test" "$OUT/cmake-build/libcounter.so" "$OUT/"
rm -rf "$OUT/cmake-build"

# Every import must have a definition on the phone.
defined=$(mktemp)
for lib in "$LIB/libc.so.3" "$LIB/libm.so.2" "$OUT/libc++_shared.so.1" "$OUT/libshape.so" "$OUT/libcounter.so"; do
    "${B}nm" -D --defined-only "$lib" | awk '{ print $3 }'
done | sort -u >"$defined"
status=0
for f in cxx20 cxx20-shared modules dlopen libc cmake-test libshape.so plugin.so libc++_shared.so.1; do
    missing=$("${B}nm" -D --undefined-only "$OUT/$f" | awk '$1 == "U" { print $2 }' | sort -u | comm -23 - "$defined")
    if [ -n "$missing" ]; then
        echo "$f: not on BB10: $missing"
        status=1
    fi
    # The unwind tables have to be in the segment the unwinder reads.
    if "${B}readelf" -SW "$OUT/$f" | grep -q '\.ARM\.exidx\.'; then
        echo "$f: stray .ARM.exidx.* sections"
        status=1
    fi
done
rm -f "$defined"
cp "$HERE/run-tests.sh" "$OUT/"
[ $status = 0 ] && echo "built: $OUT"
exit $status
