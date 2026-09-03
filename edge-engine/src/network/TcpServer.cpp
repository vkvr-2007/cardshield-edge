#include "network/TcpServer.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

TcpServer::TcpServer(
    int port,
    ThreadSafeQueue<TelemetryEvent>& event_queue)
    : port_(port),
      event_queue_(event_queue) {}

void TcpServer::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    int opt = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)) < 0) {

        std::cerr << "Failed to configure socket." << std::endl;
        close(server_fd);
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) < 0) {

        std::cerr << "Failed to bind socket." << std::endl;
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "CardShield Edge listening on port "
              << port_ << std::endl;

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_length = sizeof(client_address);

        int client_fd = accept(
            server_fd,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length
        );

        if (client_fd < 0) {
            std::cerr << "Failed to accept connection."
                      << std::endl;
            continue;
        }

        std::cout << "Client connected." << std::endl;

        char buffer[1024];

        ssize_t bytes_received = recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';

            std::cout << "Received: "
                      << buffer
                      << std::endl;

            TelemetryEvent event;
            event.source_id = "src_001";
            event.event_id = "evt_1042";
            event.timestamp = 1725260000;
            event.payment_token = "tok_demo_123";
            event.fingerprint = "fp_07";
            event.payment_failed = true;

            event_queue_.push(event);

            std::cout << "Event queued." << std::endl;
        }

        close(client_fd);
    }

    close(server_fd);
}
