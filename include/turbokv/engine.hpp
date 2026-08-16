#pragma once

#include "turbokv/types.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

namespace turbokv {

class Engine {
public:
    explicit Engine(bool enable_background_cleaner = true);
    ~Engine();

    // Core operations
    bool set(const std::string& key, const std::string& value, int64_t ttl_ms = 0);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    std::vector<std::string> keys(const std::string& prefix = "");
    void clear();

    // Statistics
    Stats get_stats() const;
    size_t size() const;

    // Purge expired keys explicitly
    size_t purge_expired();

private:
    struct Shard {
        mutable std::shared_mutex mutex;
        std::unordered_map<std::string, Entry> table;
    };

    static constexpr size_t NUM_SHARDS = 32;
    Shard shards_[NUM_SHARDS];

    Shard& get_shard(const std::string& key);
    const Shard& get_shard(const std::string& key) const;

    std::atomic<uint64_t> reads_{0};
    std::atomic<uint64_t> writes_{0};
    std::atomic<uint64_t> deletes_{0};
    std::atomic<uint64_t> expired_cleaned_{0};

    std::atomic<bool> running_{false};
    std::thread cleaner_thread_;
    void cleaner_loop();
};

} // namespace turbokv
