#pragma once

#include "turbokv/engine.hpp"
#include "turbokv/wal.hpp"
#include <string>
#include <atomic>
#include <vector>
#include <thread>

namespace turbokv {

class Server {
public:
    Server(Engine& engine, Wal* wal, int port = 6389);
    ~Server();

    bool start();
    void stop();

    // Process a single line command and return response string
    std::string process_command(const std::string& line);

private:
    Engine& engine_;
    Wal* wal_{nullptr};
    int port_;
    std::atomic<bool> running_{false};
    int server_fd_{-1};
    std::vector<std::thread> worker_threads_;

    void handle_client(int client_fd);
    std::vector<std::string> parse_tokens(const std::string& line);
};

} // namespace turbokv
