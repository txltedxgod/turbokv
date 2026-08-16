#include "turbokv/engine.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

int main() {
    constexpr size_t NUM_OPERATIONS = 500'000;
    constexpr size_t NUM_THREADS = 4;

    std::cout << "========================================\n";
    std::cout << " TurboKV In-Memory Benchmark\n";
    std::cout << " Operations: " << NUM_OPERATIONS << " | Threads: " << NUM_THREADS << "\n";
    std::cout << "========================================\n";

    turbokv::Engine engine(false);

    // Benchmark SET
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    size_t chunk = NUM_OPERATIONS / NUM_THREADS;

    for (size_t t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&engine, t, chunk]() {
            for (size_t i = 0; i < chunk; ++i) {
                std::string k = "bench_key_" + std::to_string(t * chunk + i);
                std::string v = "bench_val_" + std::to_string(i);
                engine.set(k, v);
            }
        });
    }

    for (auto& th : threads) th.join();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "[SET] Time: " << diff.count() << "s | Throughput: "
              << static_cast<size_t>(NUM_OPERATIONS / diff.count()) << " ops/sec\n";

    // Benchmark GET
    threads.clear();
    start = std::chrono::high_resolution_clock::now();
    for (size_t t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&engine, t, chunk]() {
            for (size_t i = 0; i < chunk; ++i) {
                std::string k = "bench_key_" + std::to_string(t * chunk + i);
                auto res = engine.get(k);
                (void)res;
            }
        });
    }

    for (auto& th : threads) th.join();
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "[GET] Time: " << diff.count() << "s | Throughput: "
              << static_cast<size_t>(NUM_OPERATIONS / diff.count()) << " ops/sec\n";

    return 0;
}
