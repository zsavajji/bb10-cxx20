# XIAZY's BlackBerry 10 toolchain (clang 19, the armnto binutils, the BB10
# SDK) plus a C++20 standard library: LLVM libc++, libc++abi and libunwind
# 19.1.7 built for BB10, in /opt/bb10-cxx20.
#
#   docker build -t bb10-cxx20 .
#   docker run --rm -v "$PWD:/src" -w /src bb10-cxx20 bb10-clang++ -O2 app.cpp -o app
FROM ghcr.io/xiazy/blackberry10-toolchain@sha256:60ce49510a37ed92368cca010bbaabf97b12d209b9fab179e8491b266bb2578f

COPY . /usr/src/bb10-cxx20
RUN cd /usr/src/bb10-cxx20 \
    && scripts/fetch-llvm.py /tmp/llvm/llvm-project \
    && scripts/build-runtimes.sh /tmp/llvm/llvm-project /opt/bb10-cxx20 \
    && BB10_CXX20=/opt/bb10-cxx20 tests/build.sh /opt/bb10-cxx20/tests \
    && rm -rf /tmp/llvm /tmp/bb10-cxx20-build

ENV PATH=/opt/bb10-cxx20/bin:$PATH
