#include "turbokv/engine.hpp"
#include "turbokv/wal.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

void test_basic_crud() {
    turbokv::Engine engine(false);
    assert(engine.size() == 0);

    engine.set("user:101", "alice");
    assert(engine.exists("user:101"));
    assert(engine.get("user:101").value() == "alice");

    engine.set("user:101", "alice_updated");
    assert(engine.get("user:101").value() == "alice_updated");

    assert(engine.del("user:101") == true);
    assert(!engine.exists("user:101"));
    assert(!engine.get("user:101").has_value());

    std::cout << "[PASS] test_basic_crud\n";
}

void test_ttl_expiration() {
    turbokv::Engine engine(false);
    engine.set("temp_key", "temporary_data", 50); // 50 ms TTL
    assert(engine.exists("temp_key"));

    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    assert(!engine.exists("temp_key"));
    assert(!engine.get("temp_key").has_value());

    std::cout << "[PASS] test_ttl_expiration\n";
}

void test_wal_persistence() {
    std::string test_wal = "test_persistence.wal";
    std::remove(test_wal.c_str());

    {
        turbokv::Engine engine(false);
        turbokv::Wal wal(test_wal);
        assert(wal.open());

        engine.set("hero", "batman");
        wal.log_set("hero", "batman", 0);

        engine.set("city", "gotham");
        wal.log_set("city", "gotham", 0);

        engine.del("hero");
        wal.log_del("hero");
    }

    {
        turbokv::Engine restored_engine(false);
        turbokv::Wal wal(test_wal);
        size_t replayed = wal.replay(restored_engine);

        assert(replayed == 3);
        assert(!restored_engine.exists("hero"));
        assert(restored_engine.exists("city"));
        assert(restored_engine.get("city").value() == "gotham");
    }

    std::remove(test_wal.c_str());
    std::cout << "[PASS] test_wal_persistence\n";
}

int main() {
    std::cout << "Running TurboKV Unit Tests...\n";
    test_basic_crud();
    test_ttl_expiration();
    test_wal_persistence();
    std::cout << "All tests passed successfully!\n";
    return 0;
}
