#!/bin/sh
# Builds the C++20 runtime for BB10 into PREFIX (default /opt/bb10-cxx20):
# LLVM libunwind, libc++abi and libc++, static, and all three in one
# libc++_shared.so, plus libclang_rt.profile.a (PGO) and libbb10-cxx20-rt.a
# (emulated TLS, and compiler-rt's atomic helpers, which the image's bare-metal
# builtins leave out). Runs inside XIAZY's blackberry10-toolchain image, with
# the LLVM sources at SRC (an llvm-project tree holding at least cmake, llvm,
# runtimes, libcxx, libcxxabi, libunwind and compiler-rt).
#
#   scripts/build-runtimes.sh SRC [PREFIX]
set -eu

SRC=$(cd "$1" && pwd)
PREFIX=${2:-/opt/bb10-cxx20}
HERE=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${BUILD:-/tmp/bb10-cxx20-build}
JOBS=${JOBS:-$(nproc)}

mkdir -p "$PREFIX/bin" "$PREFIX/lib" "$PREFIX/share"
cp "$HERE/bin/bb10-clang" "$PREFIX/bin/bb10-clang"
ln -sf bb10-clang "$PREFIX/bin/bb10-clang++"
cp "$HERE/cmake/bb10-cxx20.cmake" "$PREFIX/share/bb10-cxx20.cmake"
cp "$HERE/ld/bb10-exidx.ld" "$PREFIX/share/bb10-exidx.ld"
mkdir -p "$PREFIX/include"
cp -R "$HERE/include/bb10-compat" "$PREFIX/include/"
export BB10_CXX20="$PREFIX"
CC="$PREFIX/bin/bb10-clang"

# libc++ knows no QNX. Give it a locale layer that uses its "C"-locale
# fallbacks (patches/qnx.h, listed among the installed headers)...
LIBCXX="$SRC/libcxx/include"
if ! grep -q "locale_base_api/qnx.h" "$LIBCXX/__locale_dir/locale_base_api.h"; then
    cp "$HERE/patches/qnx.h" "$LIBCXX/__locale_dir/locale_base_api/qnx.h"
    sed -i 's|^#elif defined(__APPLE__) \|\| defined(__FreeBSD__)$|#elif defined(__QNX__)\n#  include <__locale_dir/locale_base_api/qnx.h>\n&|' \
        "$LIBCXX/__locale_dir/locale_base_api.h"
    sed -i 's|^\(\s*\)__locale_dir/locale_base_api/openbsd.h$|&\n\1__locale_dir/locale_base_api/qnx.h|' \
        "$LIBCXX/CMakeLists.txt"
    # ...its own ctype table, as for musl and Bionic,
    sed -i 's|^\(\s*\)defined(_LIBCPP_HAS_MUSL_LIBC) \|\| defined(__OpenBSD__)$|\1defined(_LIBCPP_HAS_MUSL_LIBC) \|\| defined(__OpenBSD__) \|\| defined(__QNX__)|' \
        "$LIBCXX/__config"
    # ...no <nl_types.h> (message catalogs),
    sed -i 's|^#\(\s*\)if !defined(__BIONIC__) \&\& !defined(_NEWLIB_VERSION) \&\& !defined(__EMSCRIPTEN__)$|#\1if !defined(__BIONIC__) \&\& !defined(_NEWLIB_VERSION) \&\& !defined(__EMSCRIPTEN__) \&\& !defined(__QNX__)|' \
        "$LIBCXX/locale"
    # ...and stat times in whole seconds and no openat() family, as on z/OS.
    sed -i 's|^#elif defined(__MVS__)$|#elif defined(__MVS__) \|\| defined(__QNX__)|' \
        "$SRC/libcxx/src/filesystem/time_utils.h"
    sed -i 's|^#if defined(_LIBCPP_WIN32API) \|\| defined(__MVS__)$|#if defined(_LIBCPP_WIN32API) \|\| defined(__MVS__) \|\| defined(__QNX__)|' \
        "$SRC/libcxx/src/filesystem/operations.cpp"
    grep -q "locale_base_api/qnx.h" "$LIBCXX/__locale_dir/locale_base_api.h"
    grep -q "locale_base_api/qnx.h" "$LIBCXX/CMakeLists.txt"
    grep -q "defined(__OpenBSD__) || defined(__QNX__)" "$LIBCXX/__config"
    grep -q "defined(__EMSCRIPTEN__) && !defined(__QNX__)" "$LIBCXX/locale"
    grep -q "defined(__MVS__) || defined(__QNX__)" "$SRC/libcxx/src/filesystem/time_utils.h"
    grep -q "defined(__MVS__) || defined(__QNX__)" "$SRC/libcxx/src/filesystem/operations.cpp"
fi

# temp_directory_path() without TMPDIR: /var/tmp, not /tmp. On BB10 /tmp links to
# /dev/shmem, where procnto's shared memory objects overlay a directory of the root
# filesystem: a nested mkdir can land on either, lookups find only procnto's entries,
# and create_directories, rename and remove_all fail. /var/tmp is an ordinary
# world-writable (1777) directory.
FS_OPS="$SRC/libcxx/src/filesystem/operations.cpp"
if ! grep -q '"/var/tmp"' "$FS_OPS"; then
    sed -i 's|^#  if defined(__ANDROID__)$|#  if defined(__QNX__)\n    ret = "/var/tmp";\n#  elif defined(__ANDROID__)|' "$FS_OPS"
    grep -q '"/var/tmp"' "$FS_OPS"
fi

# libc++abi's __cxa_end_cleanup is assembly, which -fvisibility=hidden does
# not reach: hide it too in the hermetic static library.
ABI_EXC="$SRC/libcxxabi/src/cxa_exception.cpp"
if ! grep -q "hidden	__cxa_end_cleanup" "$ABI_EXC"; then
    sed -i 's|^\(\s*\)"\t\.globl\t__cxa_end_cleanup\\n"$|&\n#if defined(_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS)\n\1"\t.hidden\t__cxa_end_cleanup\\n"\n#endif|' "$ABI_EXC"
    grep -q "hidden	__cxa_end_cleanup" "$ABI_EXC"
fi

# The catch clauses' type_info pointers are R_ARM_TARGET2 relocations, whose
# meaning each platform's linker chooses. libc++abi reads them as Linux's
# (GOT-relative: one more dereference) unless bare metal; armnto's ld writes
# them PC-relative, as QNX's GCC reads them. Read as on Linux, every catch
# by type called through a type_info's vtable and jumped to an instruction
# word (0xe92d4800).
ABI_PERS="$SRC/libcxxabi/src/cxa_personality.cpp"
if ! grep -q "LIBCXXABI_BAREMETAL) || defined(__QNX__)" "$ABI_PERS"; then
    sed -i 's|^#if defined(LIBCXXABI_BAREMETAL)$|#if defined(LIBCXXABI_BAREMETAL) \|\| defined(__QNX__)|' "$ABI_PERS"
    grep -q "LIBCXXABI_BAREMETAL) || defined(__QNX__)" "$ABI_PERS"
fi

# libbb10-cxx20-rt.a: the atomic libcalls from compiler-rt, and from src-rt/
# emulated TLS (thread_local: src-rt/emutls.c, written for QNX, instead of
# compiler-rt's emutls.c), __dso_handle, and the C11 and POSIX 2008 functions
# libc++ needs that BB10's libc lacks.
rm -rf "$BUILD" && mkdir -p "$BUILD/rt"
BB10_CXX20_NOSTDLIB=1 "$CC" -O2 -c "$SRC/compiler-rt/lib/builtins/atomic.c" \
    -o "$BUILD/rt/atomic.o" -Wno-everything
for f in "$HERE"/src-rt/*.c; do
    BB10_CXX20_NOSTDLIB=1 "$CC" -O2 -Wall -Werror -c "$f" -o "$BUILD/rt/$(basename "$f" .c).o"
done
rm -f "$PREFIX/lib/libbb10-cxx20-rt.a"
/opt/bb10-toolchain/bin/arm-unknown-nto-qnx6.5.0eabi-ar rcs "$PREFIX/lib/libbb10-cxx20-rt.a" "$BUILD"/rt/*.o

if command -v ninja >/dev/null; then GEN=Ninja; else GEN="Unix Makefiles"; fi
runtimes() { # BUILD_DIR INSTALL_PREFIX CMAKE_OPTIONS...
    dir=$1 prefix=$2
    shift 2
    BB10_CXX20_NOSTDLIB=1 cmake -G "$GEN" -S "$SRC/runtimes" -B "$dir" \
        -DCMAKE_TOOLCHAIN_FILE="$PREFIX/share/bb10-cxx20.cmake" \
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DLLVM_ENABLE_RUNTIMES="libunwind;libcxxabi;libcxx" \
        -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_DOCS=OFF \
        -DLIBUNWIND_ENABLE_SHARED=OFF -DLIBUNWIND_ENABLE_STATIC=ON \
        -DLIBUNWIND_USE_COMPILER_RT=ON -DLIBUNWIND_INCLUDE_TESTS=OFF \
        -DLIBCXXABI_ENABLE_SHARED=OFF -DLIBCXXABI_ENABLE_STATIC=ON \
        -DLIBCXXABI_USE_LLVM_UNWINDER=ON -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
        -DLIBCXXABI_USE_COMPILER_RT=ON -DLIBCXXABI_INCLUDE_TESTS=OFF \
        -DLIBCXXABI_HAS_CXA_THREAD_ATEXIT_IMPL=OFF \
        -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXX_USE_COMPILER_RT=ON \
        -DLIBCXX_HAS_PTHREAD_API=ON -DLIBCXXABI_HAS_PTHREAD_API=ON \
        `# pthreads and dlopen() are in QNX's libc, and atomics in libbb10-cxx20-rt.a` \
        -DLIBCXX_HAS_PTHREAD_LIB=OFF -DLIBCXXABI_HAS_PTHREAD_LIB=OFF -DLIBUNWIND_HAS_PTHREAD_LIB=OFF \
        -DLIBCXXABI_HAS_DL_LIB=OFF -DLIBUNWIND_HAS_DL_LIB=OFF \
        -DLIBCXX_HAS_ATOMIC_LIB=OFF -DLIBCXX_HAS_RT_LIB=OFF \
        -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF \
        -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
        -DLIBCXX_HARDENING_MODE=none "$@"
    BB10_CXX20_NOSTDLIB=1 cmake --build "$dir" -j "$JOBS" --target install
}

# libc++_shared.so: libc++ with libc++abi and libunwind linked in, exporting
# all three, for programs made of several shared libraries.
runtimes "$BUILD/shared" "$BUILD/shared-install" \
    -DLIBCXX_ENABLE_SHARED=ON -DLIBCXX_ENABLE_STATIC=OFF \
    -DLIBCXX_SHARED_OUTPUT_NAME=c++_shared -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON
cp -P "$BUILD"/shared-install/lib/libc++_shared.so* "$PREFIX/lib/"

# The static libraries, hermetic (hidden), as in Android's NDK: a module
# linked with them keeps its C++ runtime to itself, and does not take over
# the _Unwind_* functions BB10's libc and libm export (from libgcc) for the
# system libraries in the same process.
runtimes "$BUILD/static" "$PREFIX" \
    -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_STATIC=ON \
    -DLIBUNWIND_HIDE_SYMBOLS=ON -DLIBCXXABI_HERMETIC_STATIC_LIBRARY=ON \
    -DLIBCXX_HERMETIC_STATIC_LIBRARY=ON
# libclang_rt.profile.a, for -fprofile-generate and -fprofile-instr-generate:
# compiler-rt's profile runtime. For ELF, clang leaves finding the counters to
# the linker's __start_/__stop_ section symbols, which armnto's ld provides, so
# QNX takes Linux's platform file. (With InstrProfilingPlatformOther.c, which
# waits for registrations nothing makes, it links and writes empty profiles.)
PROF="$SRC/compiler-rt/lib/profile"
if ! grep -q "defined(__QNX__)" "$PROF/InstrProfilingPlatformLinux.c"; then
    sed -i 's|^\(\s*\)defined(_AIX)$|\1defined(_AIX) \|\| defined(__QNX__)|' "$PROF/InstrProfilingPlatformLinux.c"
    # (No <elf.h> in the SDK: it is only for build IDs, which go without.)
    sed -i 's|^#if !defined(_AIX)$|#if !defined(_AIX) \&\& !defined(__QNX__)|' "$PROF/InstrProfilingPlatformLinux.c"
    grep -q "defined(_AIX) || defined(__QNX__)" "$PROF/InstrProfilingPlatformLinux.c"
    grep -q "!defined(_AIX) && !defined(__QNX__)" "$PROF/InstrProfilingPlatformLinux.c"
fi
mkdir -p "$BUILD/profile"
for f in "$PROF"/*.c "$PROF"/InstrProfilingRuntime.cpp; do
    case $f in
    *PlatformLinux.c) ;;
    *Platform*|*WindowsMMap.c) continue ;;
    esac
    case $f in
    *.cpp) cc="$PREFIX/bin/bb10-clang++ -fno-exceptions -fno-rtti" ;;
    *) cc="$CC" ;;
    esac
    o=$(basename "$f")
    BB10_CXX20_NOSTDLIB=1 $cc -O2 -I"$SRC/compiler-rt/include" -I"$PROF" \
        -DCOMPILER_RT_HAS_ATOMICS=1 -DCOMPILER_RT_HAS_FCNTL_LCK=1 -DCOMPILER_RT_HAS_UNAME=1 \
        -c "$f" -o "$BUILD/profile/${o%.*}.o"
done
rm -f "$PREFIX/lib/libclang_rt.profile.a"
/opt/bb10-toolchain/bin/arm-unknown-nto-qnx6.5.0eabi-ar rcs "$PREFIX/lib/libclang_rt.profile.a" "$BUILD"/profile/*.o

# The licenses of what the prefix contains: LLVM's (Apache 2.0 with LLVM
# Exceptions) for the runtimes and compiler-rt's parts, and this
# repository's (MIT) for the driver, headers and libbb10-cxx20-rt.a's own code.
for project in libcxx libcxxabi libunwind compiler-rt; do
    mkdir -p "$PREFIX/share/licenses/$project"
    cp "$SRC/$project/LICENSE.TXT" "$PREFIX/share/licenses/$project/"
done
mkdir -p "$PREFIX/share/licenses/bb10-cxx20"
cp "$HERE/LICENSE" "$PREFIX/share/licenses/bb10-cxx20/"
echo "installed: $PREFIX"
