# bb10-cxx20

C++20 for BlackBerry 10. It extends
[XIAZY/blackberry10-toolchain](https://github.com/XIAZY/blackberry10-toolchain)
(clang 19, the armnto binutils, the BB10 10.3 SDK sysroot) with a modern C++
standard library: LLVM **libc++, libc++abi and libunwind 19.1.7**, built for
BB10 (QNX 6.6, ARMv7, softfp), and a driver, `bb10-clang++`, that compiles
`-std=gnu++20` against them.

The SDK's own C++ is GCC 4.6 or C++98 with Dinkumware's library. XIAZY's
image makes clang 19 usable for BB10 but keeps that library, so `std::span`,
`<ranges>`, `<format>`, `std::thread` and the rest of C++11–20 were not
available. With this, they are.

```sh
docker build -t bb10-cxx20 .
docker run --rm -v "$PWD:/src" -w /src bb10-cxx20 bb10-clang++ -O2 app.cpp -o app
```

## Getting it

**Docker image** (recommended). The `Dockerfile` starts from XIAZY's image
(pinned by digest), downloads the LLVM 19.1.7 release sources (checked
against their sha256), builds the runtimes into `/opt/bb10-cxx20`, builds
the tests, and puts `/opt/bb10-cxx20/bin` on `PATH`. It takes about 3
minutes.

**Prebuilt image.** `Dockerfile.release` is the same image made from a
release's tarball (checked against its sha256), without building anything:

```sh
docker build -f Dockerfile.release -t bb10-cxx20:19.1.7-2 .
```

Publishing a release also publishes that image to GitHub's registry
(`.github/workflows/docker.yml`):

```sh
docker pull ghcr.io/zsavajji/bb10-cxx20:latest
```

Both images exist for x86-64 and ARM64 (XIAZY's image has both), so on an
Apple Silicon Mac they run natively in Docker Desktop, OrbStack or Colima.

**Prebuilt tarball.** `bb10-cxx20.tar.gz` is the `/opt/bb10-cxx20` that
build produces. Unpack it into `/opt` of XIAZY's image (it needs that
image's clang, binutils and SDK at their usual paths):

```sh
docker run --rm -it -v "$PWD:/src" -w /src \
  ghcr.io/xiazy/blackberry10-toolchain@sha256:60ce49510a37ed92368cca010bbaabf97b12d209b9fab179e8491b266bb2578f \
  sh -c 'tar -xzf bb10-cxx20.tar.gz -C /opt && export PATH=/opt/bb10-cxx20/bin:$PATH && exec sh'
```

**By hand**, inside XIAZY's image:

```sh
scripts/fetch-llvm.py /tmp/llvm/llvm-project
scripts/build-runtimes.sh /tmp/llvm/llvm-project /opt/bb10-cxx20
```

## Using it

`bb10-clang` (C) and `bb10-clang++` (C++) take gcc-style arguments, like
XIAZY's `bb10-cc`, whose target flags and link line they keep.

```sh
bb10-clang++ -O2 -c engine.cpp                 # -std=gnu++20 unless -std= is given
bb10-clang++ -O2 engine.o -o engine            # static C++ runtime (the default)
bb10-clang++ -O2 -shared -o libfoo.so foo.cpp  # shared libraries too
bb10-clang++ -O2 -shared-libc++ main.cpp -o app  # with libc++_shared.so
```

CMake:

```sh
cmake -DCMAKE_TOOLCHAIN_FILE=/opt/bb10-cxx20/share/bb10-cxx20.cmake -B build
```

Autotools and plain Makefiles: `CC=bb10-clang CXX=bb10-clang++`, and
`--host=arm-unknown-nto-qnx6.5.0eabi` for configure scripts. Autoconf's
C++11 check fails under C++20 (its test assigns a `u8""` literal to
`const char *`, which C++20's `char8_t` forbids) and then appends
`-std=gnu++11` to `CXX`. Tell configure the compiler needs nothing (an
empty value means "none needed"):

```sh
./configure --host=arm-unknown-nto-qnx6.5.0eabi CC=bb10-clang CXX=bb10-clang++ \
    ac_cv_prog_cxx_cxx11=
```

Environment: `BB10_CC_VERBOSE=1` prints the clang and ld command lines;
`QNX_TARGET` points at another SDK target.

### Static or shared C++ runtime

- **Static (default).** libc++, libc++abi and libunwind go into the program,
  so nothing extra ships with it. They are built *hermetic*, with hidden
  symbols (as Android's NDK does), so each executable or shared library keeps
  its runtime to itself. This matters on BB10 because its libc and libm
  export a libgcc unwinder (`_Unwind_*`) that system libraries in the same
  process use. Do not pass C++ objects, or let exceptions escape, between
  modules that each have their own copy.
- **`-shared-libc++`.** Links `libc++_shared.so.1` (all three runtimes in one
  library, 1.5 MB) instead. Use this for programs made of several shared
  libraries that share C++ types, RTTI or exceptions. Ship
  `lib/libc++_shared.so.1` next to the program (in a .bar, under
  `native/lib`, which is on the app's `LD_LIBRARY_PATH`).

### Not for Cascades/Qt code

Qt 4.8 and Cascades on the phone are built with GCC 4.6 against the SDK's
Dinkumware library, so their C++ ABI for `std::` types and exceptions is not
libc++'s. Use this toolchain for programs and libraries that talk to the
system through C APIs (libc, Screen, EGL/GLES, BPS, sockets), such as a
browser engine process. Keep the Cascades UI on the SDK compiler, and connect
the two over a process boundary or a C interface.

## What is in it

| | |
|---|---|
| `bin/bb10-clang` | The driver (Python; `bb10-clang++` is a symlink). It compiles with clang for `armv7-none-eabi` with BB10's flags (`-mcpu=cortex-a9 -mfpu=vfpv3-d16 -mfloat-abi=softfp -D__QNX__ …`), with libc++'s headers ahead of the SDK's. It links with the armnto `ld`: `crt1.o`/`crti.o`/`crtn.o`, the C++ runtime, `libbb10-cxx20-rt.a`, `libm`, `libc` and compiler-rt's builtins. |
| `lib/libc++.a`, `libc++abi.a`, `libunwind.a` | The static runtime, hermetic. |
| `lib/libc++_shared.so.1` | The shared runtime. |
| `lib/libbb10-cxx20-rt.a` | What BB10 lacks and every program may need: compiler-rt's emulated TLS (`thread_local`) and atomic libcalls, `__dso_handle`, and libc functions QNX 6.6 lacks: `aligned_alloc`, `vasprintf`/`asprintf`, `getline`/`getdelim`, `memmem`, `memrchr`, `strchrnul`, `mkostemp`, `pipe2`, `dup3`, `madvise` (over `posix_madvise`), `malloc_usable_size` (over `_msize`), `wcsnlen`, `mbsnrtowcs`, `wcsnrtombs`. |
| `include/c++/v1` | libc++'s headers. |
| `include/bb10-compat` | Small wrappers over SDK headers that libc++ trips on (below). |
| `share/bb10-cxx20.cmake` | CMake toolchain file. |
| `share/bb10-exidx.ld` | A linker script fragment (below). |
| `tests/` | The test programs, built (in the image and the tarball). |

### How it was made to work

libc++ has no QNX support, and BB10's libc is from 2014. The changes, all
in this repository:

- **Locale.** BB10 has no `locale_t` and no `*_l` functions, so libc++ gets
  a QNX locale layer (`patches/qnx.h`) on its "C"-locale fallbacks, as for
  Fuchsia, and its own ctype table, as for musl. QNX's `LC_*` constants are
  already bit masks, so `LC_*_MASK` are redefined to them.
- **libc++ configuration** (`scripts/build-runtimes.sh` edits libc++ with
  `sed`, and checks each edit took). No `<nl_types.h>` message catalogs. In
  `<filesystem>`: stat times in whole seconds and no `openat()` family, as on
  z/OS.
- **SDK headers** (`include/bb10-compat`): `<link.h>` (QNX has
  `<sys/link.h>`, with a `const` callback for `dl_iterate_phdr`).
  `<sys/cdefs.h>` and `<sys/siginfo.h>` stop Dinkumware's habit of putting
  the C library in `namespace std` (and keep the SDK headers' include
  guards working in C++). `<math.h>` and `<wchar.h>` drop the function-like
  macros (`cos`, `log`, `wcstod`, …) that break libc++'s overloads, and
  `<math.h>` the SDK's templates for mixed argument types. `<sys/platform.h>`
  survives clang's `<stdbool.h>` coming first in C++. `<stdlib.h>`,
  `<string.h>`, `<stdio.h>`, `<wchar.h>`, `<unistd.h>`, `<malloc.h>` and
  `<sys/mman.h>` declare the functions `libbb10-cxx20-rt.a` adds (and `MADV_*`, as QNX 7
  has them). All of these, and the SDK's headers, are searched after the
  program's own `-I` and `-isystem` directories, so a library it brings
  (a newer ICU than the SDK's, say) takes precedence. `<strings.h>` is fixed for C++, and
  `<langinfo.h>` is a stub. `-D_HAS_C9X=1` shows the SDK's C99 math
  (`fpclassify`, `FP_NAN`), and `-D_NO_CPP_INLINES` hides its C++ overloads.
- **Exception tables.** The armnto linker script in XIAZY's binutils (2.45)
  has no rule for `.ARM.exidx`/`.ARM.extab`, so every inline function's
  unwind table became a section of its own outside the `PT_ARM_EXIDX` segment
  the unwinder reads, and exceptions could not unwind through those
  functions. `share/bb10-exidx.ld` adds the standard ARM rules to every link
  (`INSERT AFTER .rodata1`). `tests/build.sh` checks for this.
- **`__dso_handle`**, which GCC's `crtbegin.o` normally defines, is in
  `libbb10-cxx20-rt.a`, one hidden copy per module. A shared library runs
  its static destructors from `__cxa_finalize` when it is unloaded, as with
  `crtbeginS.o`. BB10's `crt1.o` already runs `.init_array`, so
  constructors work.
- **One emulated-TLS and atomics table per process.** Their functions have
  default visibility and executables export them, so shared libraries bind
  to one copy.
- **Thread-local storage** is emulated (`-femulated-tls`): QNX 6.6's loader
  has no ELF TLS. A `thread_local` access is a function call.
- libc++'s **experimental features are on** (`-D_LIBCPP_ENABLE_EXPERIMENTAL`),
  which in libc++ 19 means `std::jthread`, `std::stop_token` and
  `std::osyncstream`. libc++ 20 no longer marks them experimental.

## Testing

`tests/build.sh OUT` builds every test with the installed toolchain. It
then checks that every symbol each binary imports exists in BB10's
`libc.so.3`/`libm.so.2` (or in the test's own libraries), and that no unwind
table is outside `PT_ARM_EXIDX`:

| Test | Checks |
|---|---|
| `headers.c`, `headers.cpp` | the SDK's C and POSIX headers compile, twice over (include guards), in C and C++ (compile only) |
| `include-order.c` | a program's `-isystem` directory comes before the SDK's headers (compile only) |
| `libc` | the libc functions `libbb10-cxx20-rt.a` adds |
| `cxx20` | concepts, `<=>`, `std::span`, `<bit>`, ranges, `<numbers>`, `<charconv>`, `<format>`, `source_location`, containers, `variant`/`optional`, RTTI, exceptions (nested too), `std::thread`, atomics (64-bit too), `thread_local`, mutexes, `condition_variable`, `jthread`/`stop_token`, `latch`/`barrier`/`semaphore`, `atomic::wait`, `call_once`, iostreams, `<regex>`, `<filesystem>` |
| `cxx20-shared` | the same, with `-shared-libc++` |
| `modules` + `libshape.so` | with `-shared-libc++`: virtual calls, RTTI, an exception thrown in a library and caught in the program by its base class, `thread_local` across the boundary |
| `dlopen` + `plugin.so` | a C program loads a C++ plugin with the static runtime, which uses exceptions inside. `dlclose()` runs the plugin's static destructors |
| `cmake-test` | the CMake toolchain file: a C++20 program, a static and a shared library, `Threads::Threads` |

On the phone, copy the folder (`/opt/bb10-cxx20/tests` in the image or the
tarball) over, for example to `/accounts/1000/shared/misc/bb10-cxx20-tests`,
and from a terminal (term51, or SSH):

```sh
cp -R /accounts/1000/shared/misc/bb10-cxx20-tests ~/ && cd ~/bb10-cxx20-tests
chmod +x * && sh run-tests.sh
```

(The copy into the terminal's home is for the execute permission, which
files on the shared storage may not get.)

Each test prints one line per check and ends with `all passed` or `FAILED`.

### Status

Everything builds and links. The checks above pass: every import is found on
BB10, the unwind tables are complete, and the static runtime exports nothing.
**The tests have not been run on a phone yet**, because QNX binaries cannot
run anywhere else. The parts only a phone can confirm are exception
unwinding (LLVM's libunwind finding tables through QNX's `dl_iterate_phdr`),
emulated TLS, and `__cxa_finalize` on `dlclose`.

## Limitations

- **"C" locale only.** `std::locale("")` and named locales throw; UTF-8
  text is fine as bytes. (BB10's libc has no `newlocale`.)
- No `std::chrono` time zone database (`tzdb`), and no `from_chars` for
  floating point (libc++ 20).
- File times in `<filesystem>` have one-second resolution, and
  `remove_all` works by path (no `openat`).
- `thread_local` costs a function call per access (emulated TLS).
- XIAZY's driver adds `-lbps` to executables; this one does not. Add it if
  you use BPS.
- ARM only (the phones). XIAZY's image also targets the x86 simulator; the
  runtimes here are not built for it.

## Layout

```
bin/bb10-clang            driver (bb10-clang++ links to it)
cmake/bb10-cxx20.cmake    CMake toolchain file
include/bb10-compat/      SDK header wrappers
ld/bb10-exidx.ld          ARM unwind table rules for the armnto linker
patches/qnx.h             libc++ locale layer for QNX
src-rt/                   libbb10-cxx20-rt.a sources (besides compiler-rt's)
scripts/fetch-llvm.py     download and check the LLVM 19.1.7 sources
scripts/build-runtimes.sh patch and build the runtimes, install a prefix
tests/                    test programs, build.sh, run-tests.sh
Dockerfile                XIAZY's image + this, built from source
Dockerfile.release        XIAZY's image + a release's prebuilt tarball
.github/workflows/        publishes Dockerfile.release's image to ghcr.io
```

## License

This repository is under the MIT License (`LICENSE`). LLVM's libc++,
libc++abi, libunwind and compiler-rt, which the build downloads and the
prebuilt runtimes are made from, are under the Apache License 2.0 with LLVM
Exceptions; the prebuilt runtimes carry that license. An installed prefix
has both in `share/licenses/`. Programs linked with the runtimes need not
carry LLVM's license (that is what the LLVM Exceptions are for).
