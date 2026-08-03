# C++26 dev image (03/08/26).
FROM docker.io/library/gcc:15


RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake ninja-build git gdb ccache pkg-config python3 \
        clang clangd clang-tidy clang-format \
        lld mold lldb \
        cppcheck valgrind \
    && rm -rf /var/lib/apt/lists/*

# ccache cache lives outside the bind mount so it survives `./dev down`.
ENV CCACHE_DIR=/tmp/ccache
WORKDIR /work

# Sanity: fail the build loudly if the base image ever regresses below C++26.
RUN printf 'int main(){ return 0; }\n' > /tmp/probe.cpp \
    && g++ -std=c++2c /tmp/probe.cpp -o /tmp/probe \
    && rm -f /tmp/probe.cpp /tmp/probe
