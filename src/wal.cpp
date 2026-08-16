#include "turbokv/wal.hpp"
#include <iostream>
#include <sstream>
#include <cstdio>

namespace turbokv {

Wal::Wal(std::string path) : path_(std::move(path)) {}

Wal::~Wal() {
    close();
}

bool Wal::open() {
    std::lock_guard<std::mutex> lock(mutex_);
    out_.open(path_, std::ios::out | std::ios::app | std::ios::binary);
    return out_.is_open();
}

void Wal::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (out_.is_open()) {
        out_.flush();
        out_.close();
    }
}

void Wal::write_record(WalOp op, const std::string& key, const std::string& value, int64_t ttl_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!out_.is_open()) return;

    auto op_byte = static_cast<uint8_t>(op);
    out_.write(reinterpret_cast<const char*>(&op_byte), sizeof(op_byte));

    uint32_t key_len = static_cast<uint32_t>(key.size());
    out_.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
    out_.write(key.data(), key_len);

    if (op == WalOp::SET) {
        uint32_t val_len = static_cast<uint32_t>(value.size());
        out_.write(reinterpret_cast<const char*>(&val_len), sizeof(val_len));
        out_.write(value.data(), val_len);
        out_.write(reinterpret_cast<const char*>(&ttl_ms), sizeof(ttl_ms));
    }

    out_.flush();
}

void Wal::log_set(const std::string& key, const std::string& value, int64_t ttl_ms) {
    write_record(WalOp::SET, key, value, ttl_ms);
}

void Wal::log_del(const std::string& key) {
    write_record(WalOp::DEL, key);
}

void Wal::log_clear() {
    write_record(WalOp::CLEAR, "");
}

size_t Wal::replay(Engine& engine) {
    std::ifstream in(path_, std::ios::in | std::ios::binary);
    if (!in.is_open()) return 0;

    size_t operations = 0;
    while (in.peek() != EOF) {
        uint8_t op_byte = 0;
        in.read(reinterpret_cast<char*>(&op_byte), sizeof(op_byte));
        if (!in) break;

        auto op = static_cast<WalOp>(op_byte);

        uint32_t key_len = 0;
        in.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        if (!in) break;

        std::string key(key_len, '\0');
        in.read(&key[0], key_len);
        if (!in) break;

        if (op == WalOp::SET) {
            uint32_t val_len = 0;
            in.read(reinterpret_cast<char*>(&val_len), sizeof(val_len));
            if (!in) break;

            std::string value(val_len, '\0');
            in.read(&value[0], val_len);
            if (!in) break;

            int64_t ttl_ms = 0;
            in.read(reinterpret_cast<char*>(&ttl_ms), sizeof(ttl_ms));
            if (!in) break;

            engine.set(key, value, ttl_ms);
            operations++;
        } else if (op == WalOp::DEL) {
            engine.del(key);
            operations++;
        } else if (op == WalOp::CLEAR) {
            engine.clear();
            operations++;
        }
    }
    return operations;
}

bool Wal::compact(const Engine& engine) {
    std::string tmp_path = path_ + ".tmp";
    {
        Wal tmp_wal(tmp_path);
        if (!tmp_wal.open()) return false;

        auto all_keys = const_cast<Engine&>(engine).keys();
        for (const auto& k : all_keys) {
            auto val = const_cast<Engine&>(engine).get(k);
            if (val) {
                tmp_wal.log_set(k, *val, 0);
            }
        }
        tmp_wal.close();
    }

    close();
    std::remove(path_.c_str());
    std::rename(tmp_path.c_str(), path_.c_str());
    return open();
}

} // namespace turbokv
