# C++26 dev image (03/08/26).
FROM docker.io/library/gcc:15


RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake ninja-build git gdb ccache pkg-config python3 \
        python3-numpy netcat-openbsd \
        clang clangd clang-tidy clang-format \
        lld mold lldb \
        cppcheck valgrind \
    && rm -rf /var/lib/apt/lists/*

# ARM cross-toolchain for the Zynq-7030 (Cortex-A9, armhf) board target.
ARG ARM_TC=arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-linux-gnueabihf
RUN apt-get update && apt-get install -y --no-install-recommends \
        curl xz-utils ca-certificates \
    && curl -fsSL "https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/${ARM_TC}.tar.xz" \
         -o /tmp/arm-tc.tar.xz \
    && mkdir -p /opt && tar -xJf /tmp/arm-tc.tar.xz -C /opt && rm /tmp/arm-tc.tar.xz \
    && ln -s "/opt/${ARM_TC}" /opt/arm-cross \
    && rm -rf /var/lib/apt/lists/*
ENV PATH=/opt/arm-cross/bin:$PATH
# `./dev '<cmd>'` runs a login shell (bash -lc), which resets PATH via
# /etc/profile and drops the ENV above - re-add the toolchain there too so
# the cross compiler is visible without a manual export.
RUN printf 'export PATH=/opt/arm-cross/bin:$PATH\n' > /etc/profile.d/arm-cross.sh

# Sanity: the cross g++ must speak C++26, else the board build is pointless.
RUN printf 'int main(){}\n' > /tmp/p.cpp \
    && arm-none-linux-gnueabihf-g++ -std=c++2c /tmp/p.cpp -c -o /tmp/p.o \
    && rm -f /tmp/p.cpp /tmp/p.o

# ccache cache lives outside the bind mount so it survives `./dev down`.
ENV CCACHE_DIR=/tmp/ccache
WORKDIR /work

# Sanity: fail the build loudly if the base image ever regresses below C++26.
RUN printf 'int main(){ return 0; }\n' > /tmp/probe.cpp \
    && g++ -std=c++2c /tmp/probe.cpp -o /tmp/probe \
    && rm -f /tmp/probe.cpp /tmp/probe
