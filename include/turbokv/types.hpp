#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <cstdint>

namespace turbokv {

struct Entry {
    std::string value;
    int64_t expires_at{0}; // 0 = never expires (unix epoch millis)

    [[nodiscard]] bool is_expired() const {
        if (expires_at == 0) return false;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return now >= expires_at;
    }
};

struct Stats {
    uint64_t total_keys{0};
    uint64_t total_reads{0};
    uint64_t total_writes{0};
    uint64_t total_deletes{0};
    uint64_t expired_keys_cleaned{0};
};

} // namespace turbokv
