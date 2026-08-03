# lqr-bridge

Low-latency PS→PL gain/weight bridge for the PandA LQR controller.

## Purpose

A standalone place to test core features before forking PandABlocks-FPGA/server.

## Architecture

optimsier core -> quantiser / encoder -> transportation.

## Environment

Containerised C++26 (GCC 15 / Clang, `-std=c++2c`).

## Build & test

```sh
`cmake --build build/asan && ctest --preset asan --output-on-failure`
```

| preset  | purpose |
|---------|---------|
| debug   | plain Debug (clangd reads `build/debug`) |
| asan    | AddressSanitizer |
| tsan    | ThreadSanitizer |
| ubsan   | UndefinedBehaviorSanitizer |
| release | RelWithDebInfo |
| clang   | build under Clang (dual-compiler check) |

