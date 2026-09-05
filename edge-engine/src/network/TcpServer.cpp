#include "network/TcpServer.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

TcpServer::TcpServer(
    int port,
    ThreadSafeQueue<TelemetryEvent>& event_queue,
    std::atomic<bool>& stop_flag)
    : port_(port), event_queue_(event_queue), stop_flag_(stop_flag) {}

void TcpServer::start() {
#ifdef _WIN32
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        stop_flag_.store(true);
        return;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port_);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        stop_flag_.store(true);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed." << std::endl;
        stop_flag_.store(true);
        return;
    }

    std::cout << "TCP server listening on port "
              << port_
              << std::endl;

    while (!stop_flag_.load()) {
        sockaddr_in client_address{};
#ifdef _WIN32
        int client_length = sizeof(client_address);
#else
        socklen_t client_length = sizeof(client_address);
#endif

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(server_fd, &read_set);
        timeval timeout{};
        timeout.tv_usec = 100000;

    #ifdef _WIN32
        int ready = select(0, &read_set, nullptr, nullptr, &timeout);
    #else
        int ready = select(
            server_fd + 1,
            &read_set,
            nullptr,
            nullptr,
            &timeout);
    #endif

        if (ready <= 0) {
            continue;
        }

        int client_fd = accept(
            server_fd,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length);

        if (client_fd < 0) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        char buffer[1024];
        std::string pending_line;
        int bytes_received = 0;

    #ifdef _WIN32
        DWORD receive_timeout = 100;
        setsockopt(
            client_fd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char*>(&receive_timeout),
            sizeof(receive_timeout));
    #else
        timeval receive_timeout{};
        receive_timeout.tv_usec = 100000;
        setsockopt(
            client_fd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &receive_timeout,
            sizeof(receive_timeout));
    #endif

        while (!stop_flag_.load() && (bytes_received = recv(
                    client_fd,
                    buffer,
                    sizeof(buffer),
                    0)) > 0) {
            pending_line.append(buffer, bytes_received);

            std::size_t newline_position = 0;
            while ((newline_position = pending_line.find('\n')) !=
                   std::string::npos) {
                std::string line = pending_line.substr(0, newline_position);
                pending_line.erase(0, newline_position + 1);

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                std::optional<TelemetryEvent> event = parse_line(line);
                if (event.has_value()) {
                    event_queue_.push(*event);
                } else {
                    std::cerr << "Warning: invalid telemetry event received."
                              << std::endl;
                }
            }
        }

#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}