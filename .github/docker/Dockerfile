# Self-hosted GitHub Actions runner image for the Jde OpcGateway build.
#
# Layers the clang++-22 / libc++ toolchain (and the system libraries the -jde
# CMake presets link against) on top of the myoung34 auto-registering runner.
#
# Third-party dependencies (Boost, protobuf, open62541, jsonnet, …) are NOT baked
# into the image — they are the 2 GB $REPO_DIR/install tree and are bind-mounted
# read-only at `docker run` time. See README.md in this directory.
#
# GLIBC / ABI NOTE: the mounted deps (and tools like protoc) were built on the
# host; their highest symbol requirement across the whole tree is GLIBC_2.38, so
# the base image needs glibc >= 2.38. Ubuntu 24.04 "noble" (glibc 2.39) clears it;
# focal (2.31) and jammy (2.35) do NOT — and myoung34's `latest` tag is focal,
# which additionally ships a too-old cmake. If you later hit `GLIBC_x.y not found`,
# a dep was rebuilt against a newer glibc than the base — bump BASE_IMAGE, or
# rebuild the deps in-container with the `-repos` preset instead of mounting them.
ARG BASE_IMAGE=myoung34/github-runner:ubuntu-noble
FROM ${BASE_IMAGE}

ARG LLVM_VERSION=22
USER root

# clang/libc++/lld from apt.llvm.org, plus the system -dev packages the presets
# reference: libxml2 (LIBXML2_INCLUDE_DIR), liburing (fwk io_uring), OpenSSL, and
# zstd + zlib static libs (open62541's export links static system OpenSSL, which
# drags in -lzstd and libz.a). libbacktrace and libjitterentropy are built from
# source below (no usable apt package on noble).
RUN set -eux; \
    export DEBIAN_FRONTEND=noninteractive; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        wget gnupg lsb-release ca-certificates software-properties-common; \
    wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh; \
    chmod +x /tmp/llvm.sh; \
    /tmp/llvm.sh ${LLVM_VERSION}; \
    apt-get install -y --no-install-recommends \
        clang-${LLVM_VERSION} \
        lld-${LLVM_VERSION} \
        libc++-${LLVM_VERSION}-dev \
        libc++abi-${LLVM_VERSION}-dev \
        ninja-build make git pkg-config \
        libxml2-dev liburing-dev libssl-dev libzstd-dev zlib1g-dev; \
    rm -rf /var/lib/apt/lists/* /tmp/llvm.sh

# libbacktrace (BOOST_STACKTRACE_USE_BACKTRACE / the presets' -lbacktrace): Ubuntu
# added the libbacktrace-dev package in 25.04, so noble (24.04) lacks it — build
# from source. Built PIC because the project links it into shared libs (libJde.so),
# and installed to the multiarch libdir + /usr/include to match the host package's
# layout, so -lbacktrace and <backtrace.h> resolve with no extra search paths.
RUN set -eux; \
    git clone --depth 1 https://github.com/ianlancetaylor/libbacktrace.git /tmp/libbacktrace; \
    cd /tmp/libbacktrace; \
    ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu \
        CC=clang-${LLVM_VERSION} CFLAGS="-fPIC -O2"; \
    make -j"$(nproc)"; \
    make install; \
    rm -rf /tmp/libbacktrace

# libjitterentropy: open62541's exported link interface (baked on the host, which
# links static system OpenSSL) requires -l:libjitterentropy.a / -ljitterentropy.
# noble packages only the jitterentropy-rngd daemon, not the library, so build it
# from source. Installed to /usr (linker searches /usr/lib) as both static+shared.
RUN set -eux; \
    git clone --depth 1 https://github.com/smuellerDD/jitterentropy-library.git /tmp/jent; \
    cd /tmp/jent; \
    make -j"$(nproc)" CC=clang-${LLVM_VERSION}; \
    make install PREFIX=/usr; \
    ldconfig; \
    rm -rf /tmp/jent

# cmake from the Kitware release rather than apt: noble ships 3.28.3 — exactly the
# preset's floor — whereas the host builds on cmake 4.x. Pin to match the host.
ARG CMAKE_VERSION=4.2.3
RUN set -eux; \
    wget -qO /tmp/cmake.tar.gz \
        "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"; \
    tar -xzf /tmp/cmake.tar.gz --strip-components=1 -C /usr/local; \
    rm /tmp/cmake.tar.gz; \
    cmake --version

# The -jde presets set CMAKE_CXX_COMPILER=clang++ and link with -fuse-ld=lld, so
# the unversioned names must resolve to the v22 binaries (mirrors the host, where
# /usr/bin/clang++ -> clang++-22).
RUN set -eux; \
    update-alternatives --install /usr/bin/clang   clang   /usr/bin/clang-${LLVM_VERSION}   100; \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100; \
    ln -sf /usr/bin/lld-${LLVM_VERSION}    /usr/bin/lld; \
    ln -sf /usr/bin/ld.lld-${LLVM_VERSION} /usr/bin/ld.lld; \
    clang++ --version

# Entry point / runner registration are inherited from the base image.
