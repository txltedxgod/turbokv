# turbokv

High-performance, lightweight in-memory key-value database with TTL expiration, write-ahead logging (WAL) persistence, and a multi-threaded TCP server built with modern **C++17**.

## Features

- **Blazing Fast In-Memory Storage:** Sharded table architecture with read/write shared mutexes to reduce lock contention.
- **TTL Support:** Per-key expiration with background scavenger thread.
- **Write-Ahead Logging (WAL):** Append-only binary log with automatic replay on startup and log compaction.
- **TCP Protocol:** Redis-like text wire protocol compatible with `nc`, `telnet`, or simple TCP clients.
- **Modern C++17:** Clean codebase with zero external third-party dependencies.
- **Cross-Platform:** Builds on Linux, macOS, and Windows.

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
# Run test suite
ctest --output-on-failure
# Or directly:
./turbokv_tests

# Run benchmark
./turbokv_bench
```

## Protocol Examples

Connect with `nc` or `telnet`:

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

## Project Structure

```
├── CMakeLists.txt
├── include/
│   └── turbokv/
│       ├── engine.hpp      # Core sharded in-memory engine
│       ├── server.hpp      # Non-blocking / threaded TCP server
│       ├── types.hpp       # Data structs and stats
│       └── wal.hpp         # Binary WAL persistence
├── src/
│   ├── engine.cpp
│   ├── main.cpp
│   ├── server.cpp
│   └── wal.cpp
├── tests/
│   └── test_engine.cpp
└── benchmarks/
    └── bench.cpp
```
