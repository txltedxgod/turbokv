#include "turbokv/engine.hpp"
#include <chrono>
#include <functional>

namespace turbokv {

Engine::Engine(bool enable_background_cleaner) {
    if (enable_background_cleaner) {
        running_ = true;
        cleaner_thread_ = std::thread(&Engine::cleaner_loop, this);
    }
}

Engine::~Engine() {
    if (running_) {
        running_ = false;
        if (cleaner_thread_.joinable()) {
            cleaner_thread_.join();
        }
    }
}

Engine::Shard& Engine::get_shard(const std::string& key) {
    size_t hash = std::hash<std::string>{}(key);
    return shards_[hash % NUM_SHARDS];
}

const Engine::Shard& Engine::get_shard(const std::string& key) const {
    size_t hash = std::hash<std::string>{}(key);
    return shards_[hash % NUM_SHARDS];
}

bool Engine::set(const std::string& key, const std::string& value, int64_t ttl_ms) {
    auto& shard = get_shard(key);
    int64_t expires = 0;
    if (ttl_ms > 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        expires = now + ttl_ms;
    }

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.table[key] = Entry{value, expires};
    writes_++;
    return true;
}

std::optional<std::string> Engine::get(const std::string& key) {
    auto& shard = get_shard(key);
    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.table.find(key);
    if (it == shard.table.end()) {
        return std::nullopt;
    }

    if (it->second.is_expired()) {
        return std::nullopt;
    }

    reads_++;
    return it->second.value;
}

bool Engine::del(const std::string& key) {
    auto& shard = get_shard(key);
    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.table.find(key);
    if (it != shard.table.end()) {
        shard.table.erase(it);
        deletes_++;
        return true;
    }
    return false;
}

bool Engine::exists(const std::string& key) {
    auto& shard = get_shard(key);
    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.table.find(key);
    if (it == shard.table.end()) return false;
    return !it->second.is_expired();
}

std::vector<std::string> Engine::keys(const std::string& prefix) {
    std::vector<std::string> result;
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::shared_lock<std::shared_mutex> lock(shards_[i].mutex);
        for (const auto& [k, entry] : shards_[i].table) {
            if (!entry.is_expired()) {
                if (prefix.empty() || k.rfind(prefix, 0) == 0) {
                    result.push_back(k);
                }
            }
        }
    }
    return result;
}

void Engine::clear() {
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::unique_lock<std::shared_mutex> lock(shards_[i].mutex);
        shards_[i].table.clear();
    }
}

size_t Engine::size() const {
    size_t total = 0;
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::shared_lock<std::shared_mutex> lock(shards_[i].mutex);
        for (const auto& [k, entry] : shards_[i].table) {
            if (!entry.is_expired()) total++;
        }
    }
    return total;
}

Stats Engine::get_stats() const {
    return Stats{
        size(),
        reads_.load(),
        writes_.load(),
        deletes_.load(),
        expired_cleaned_.load()
    };
}

size_t Engine::purge_expired() {
    size_t purged = 0;
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::unique_lock<std::shared_mutex> lock(shards_[i].mutex);
        for (auto it = shards_[i].table.begin(); it != shards_[i].table.end();) {
            if (it->second.is_expired()) {
                it = shards_[i].table.erase(it);
                purged++;
            } else {
                ++it;
            }
        }
    }
    expired_cleaned_ += purged;
    return purged;
}

void Engine::cleaner_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!running_) break;
        purge_expired();
    }
}

} // namespace turbokv
