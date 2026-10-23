#!/bin/sh
# On the phone, in the folder build.sh filled: runs every test.
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$PWD:${LD_LIBRARY_PATH:-}"
failed=0
for t in unwind-probe exceptions exceptions-sysunwind cxx20 cxx20-shared modules "dlopen $PWD/plugin.so" libc cmake-test; do
    echo "== $t"
    ./$t || failed=$((failed + 1))
done
echo
[ $failed = 0 ] && echo "every test passed" || echo "$failed test(s) FAILED"
exit $failed
