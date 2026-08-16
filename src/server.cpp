#include "turbokv/server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    using socket_t = int;
    #define CLOSE_SOCKET(s) close(s)
#endif

namespace turbokv {

Server::Server(Engine& engine, Wal* wal, int port)
    : engine_(engine), wal_(wal), port_(port) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

Server::~Server() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

std::vector<std::string> Server::parse_tokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string Server::process_command(const std::string& line) {
    auto tokens = parse_tokens(line);
    if (tokens.empty()) return "+OK\r\n";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "PING") {
        return "+PONG\r\n";
    } else if (cmd == "SET") {
        if (tokens.size() < 3) {
            return "-ERR wrong number of arguments for 'SET'\r\n";
        }
        std::string key = tokens[1];
        std::string val = tokens[2];
        int64_t ttl_ms = 0;
        if (tokens.size() >= 5) {
            std::string opt = tokens[3];
            std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);
            if (opt == "EX") {
                ttl_ms = std::stoll(tokens[4]) * 1000;
            } else if (opt == "PX") {
                ttl_ms = std::stoll(tokens[4]);
            }
        }
        engine_.set(key, val, ttl_ms);
        if (wal_) wal_->log_set(key, val, ttl_ms);
        return "+OK\r\n";
    } else if (cmd == "GET") {
        if (tokens.size() != 2) {
            return "-ERR wrong number of arguments for 'GET'\r\n";
        }
        auto val = engine_.get(tokens[1]);
        if (!val) {
            return "$-1\r\n"; // nil
        }
        return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
    } else if (cmd == "DEL") {
        if (tokens.size() != 2) {
            return "-ERR wrong number of arguments for 'DEL'\r\n";
        }
        bool ok = engine_.del(tokens[1]);
        if (ok && wal_) wal_->log_del(tokens[1]);
        return ":" + std::to_string(ok ? 1 : 0) + "\r\n";
    } else if (cmd == "EXISTS") {
        if (tokens.size() != 2) {
            return "-ERR wrong number of arguments for 'EXISTS'\r\n";
        }
        bool ok = engine_.exists(tokens[1]);
        return ":" + std::to_string(ok ? 1 : 0) + "\r\n";
    } else if (cmd == "KEYS") {
        std::string prefix = (tokens.size() >= 2) ? tokens[1] : "";
        if (prefix == "*") prefix = "";
        auto list = engine_.keys(prefix);
        std::string resp = "*" + std::to_string(list.size()) + "\r\n";
        for (const auto& k : list) {
            resp += "$" + std::to_string(k.size()) + "\r\n" + k + "\r\n";
        }
        return resp;
    } else if (cmd == "DBSIZE") {
        return ":" + std::to_string(engine_.size()) + "\r\n";
    } else if (cmd == "FLUSHDB") {
        engine_.clear();
        if (wal_) wal_->log_clear();
        return "+OK\r\n";
    } else if (cmd == "STATS") {
        auto stats = engine_.get_stats();
        std::string s = "keys:" + std::to_string(stats.total_keys) +
                        " reads:" + std::to_string(stats.total_reads) +
                        " writes:" + std::to_string(stats.total_writes) +
                        " deletes:" + std::to_string(stats.total_deletes);
        return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
    } else if (cmd == "QUIT") {
        return "+BYE\r\n";
    }

    return "-ERR unknown command '" + tokens[0] + "'\r\n";
}

void Server::handle_client(int client_fd) {
    char buffer[4096];
    std::string line_buf;

    while (running_) {
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        line_buf += buffer;

        size_t pos = 0;
        while ((pos = line_buf.find('\n')) != std::string::npos) {
            std::string line = line_buf.substr(0, pos);
            line_buf.erase(0, pos + 1);

            // trim carriage return
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) continue;

            std::string resp = process_command(line);
            send(client_fd, resp.data(), static_cast<int>(resp.size()), 0);

            if (line == "QUIT" || line == "quit") {
                CLOSE_SOCKET(client_fd);
                return;
            }
        }
    }
    CLOSE_SOCKET(client_fd);
}

bool Server::start() {
    socket_t sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    int opt = 1;
#ifndef _WIN32
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed on port " << port_ << "\n";
        CLOSE_SOCKET(sfd);
        return false;
    }

    if (listen(sfd, 128) < 0) {
        std::cerr << "Listen failed\n";
        CLOSE_SOCKET(sfd);
        return false;
    }

    server_fd_ = static_cast<int>(sfd);
    running_ = true;
    std::cout << "[TurboKV] Listening on port " << port_ << "...\n";

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        socket_t cfd = accept(sfd, (struct sockaddr*)&client_addr, &len);
        if (cfd < 0) {
            if (!running_) break;
            continue;
        }

        std::thread([this, cfd]() {
            this->handle_client(static_cast<int>(cfd));
        }).detach();
    }

    return true;
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        CLOSE_SOCKET(server_fd_);
        server_fd_ = -1;
    }
}

} // namespace turbokv
