# lqr-bridge

Low-latency PS->PL gain/weight bridge for the PandA LQR controller. Runs on a forked build of the PandA-Server. Docs TODO.

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
