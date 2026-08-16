#pragma once

#include "turbokv/engine.hpp"
#include <string>
#include <fstream>
#include <mutex>

namespace turbokv {

enum class WalOp : uint8_t {
    SET = 1,
    DEL = 2,
    CLEAR = 3
};

class Wal {
public:
    explicit Wal(std::string path);
    ~Wal();

    bool open();
    void close();

    void log_set(const std::string& key, const std::string& value, int64_t ttl_ms);
    void log_del(const std::string& key);
    void log_clear();

    // Replay log into engine on startup
    size_t replay(Engine& engine);

    // Compact log by snapshotting current engine state
    bool compact(const Engine& engine);

private:
    std::string path_;
    std::ofstream out_;
    std::mutex mutex_;

    void write_record(WalOp op, const std::string& key, const std::string& value = "", int64_t ttl_ms = 0);
};

} // namespace turbokv
