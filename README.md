# turbokv

> High-performance in-memory key-value store with TTL, write-ahead logging (WAL), and TCP protocol in modern C++17.

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?style=flat-square&logo=cmake)](https://cmake.org)
[![Build](https://img.shields.io/badge/Build-Passing-brightgreen?style=flat-square)](https://github.com/txltedxgod/turbokv)
[![Docker](https://img.shields.io/badge/Docker-Ready-2496ED?style=flat-square&logo=docker)](https://docker.com)
[![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)](LICENSE)

`#key-value-store` `#database-engine` `#cpp17` `#wal` `#in-memory` `#tcp-server` `#multithreading` `#systems-programming`

---

## Features

- **Blazing Fast In-Memory Storage:** Sharded table architecture with read/write shared mutexes to minimize lock contention.
- **TTL Expiration:** Per-key expiration with automatic background scavenger thread.
- **Write-Ahead Logging (WAL):** Append-only binary log with automatic replay on startup and log compaction.
- **TCP Wire Protocol:** Redis-compatible text protocol supporting standard network clients (`nc`, `telnet`, SDKs).
- **Zero External Dependencies:** Built entirely with modern C++ Standard Library.
- **Cross-Platform:** Compiles seamlessly on Linux, macOS, and Windows.

## Quick Start

### Build with CMake

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Run Server

```bash
./turbokv_server --port 6389 --wal turbokv.wal
```

### Run Benchmarks & Tests

```bash
# Run unit tests
./turbokv_tests

# Run multi-threaded throughput benchmark
./turbokv_bench
```

## Protocol Usage

```bash
$ nc localhost 6389
PING
+PONG

SET user:1001 "john_doe" EX 60
+OK

GET user:1001
$8
john_doe

EXISTS user:1001
:1

KEYS user:*
*1
$9
user:1001

STATS
$42
keys:1 reads:1 writes:1 deletes:0

DEL user:1001
:1

QUIT
+BYE
```

## Docker

```bash
docker build -t turbokv .
docker run -d -p 6389:6389 -v $(pwd)/data:/app/data turbokv
```
