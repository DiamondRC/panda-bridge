# lqr-bridge

Low-latency PS→PL gain/weight bridge for the PandA LQR controller.

## Purpose

A standalone place to test core features before forking PandABlocks-FPGA/server.

## Environment

Containerised C++26 (GCC 15 / Clang, `-std=c++2c`).

## Build & test

```sh
./dev 'cmake --preset asan'
./dev 'cmake --build build/asan'
./dev 'ctest --preset asan'
```

| preset  | purpose |
|---------|---------|
| debug   | plain Debug (clangd reads `build/debug`) |
| asan    | AddressSanitizer |
| tsan    | ThreadSanitizer |
| ubsan   | UndefinedBehaviorSanitizer |
| release | RelWithDebInfo |
| clang   | build under Clang (dual-compiler check) |

