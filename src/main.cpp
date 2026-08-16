#include "turbokv/engine.hpp"
#include "turbokv/wal.hpp"
#include "turbokv/server.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    int port = 6389;
    std::string wal_path = "turbokv.wal";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--wal" && i + 1 < argc) {
            wal_path = argv[++i];
        }
    }

    std::cout << "======================================\n";
    std::cout << "  TurboKV Server - v1.0.0 (C++17)     \n";
    std::cout << "======================================\n";

    turbokv::Engine engine(true);
    turbokv::Wal wal(wal_path);

    if (wal.open()) {
        std::cout << "[TurboKV] Replaying WAL from " << wal_path << "...\n";
        size_t restored = wal.replay(engine);
        std::cout << "[TurboKV] Restored " << restored << " operations (" << engine.size() << " active keys)\n";
    }

    turbokv::Server server(engine, &wal, port);
    server.start();

    return 0;
}
