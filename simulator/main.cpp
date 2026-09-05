#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
constexpr Socket INVALID_SOCKET_VALUE = -1;
#endif

namespace {

constexpr int USER_COUNT = 5;
constexpr int ATTACK_BOT_COUNT = 2;
constexpr char SERVER_HOST[] = "127.0.0.1";
constexpr int SERVER_PORT = 8080;
constexpr char ALPHANUMERIC[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void close_socket(Socket socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

bool send_event(const nlohmann::json& event) {
    Socket socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == INVALID_SOCKET_VALUE) {
        return false;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_HOST, &server_address.sin_addr);

    if (connect(
            socket,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)) < 0) {
        close_socket(socket);
        return false;
    }

    const std::string payload = event.dump() + "\n";
    std::size_t bytes_sent = 0;
    while (bytes_sent < payload.size()) {
        const int result = send(
            socket,
            payload.data() + bytes_sent,
            static_cast<int>(payload.size() - bytes_sent),
            0);
        if (result <= 0) {
            close_socket(socket);
            return false;
        }
        bytes_sent += static_cast<std::size_t>(result);
    }

#ifdef _WIN32
    shutdown(socket, SD_SEND);
#else
    shutdown(socket, SHUT_WR);
#endif
    close_socket(socket);
    return true;
}

std::string random_identifier(
    std::mt19937& generator,
    std::size_t length) {
    std::uniform_int_distribution<std::size_t> distribution(
        0,
        sizeof(ALPHANUMERIC) - 2);
    std::string identifier;
    identifier.reserve(length);

    for (std::size_t index = 0; index < length; ++index) {
        identifier.push_back(ALPHANUMERIC[distribution(generator)]);
    }

    return identifier;
}

void run_user(
    int user_index,
    std::chrono::seconds duration,
    std::atomic<std::uint64_t>& event_counter) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_real_distribution<double> mean_distribution(3.0, 8.0);
    std::bernoulli_distribution failure_distribution(0.05);
    const double mean_seconds = mean_distribution(generator);
    std::exponential_distribution<double> interval_distribution(
        1.0 / mean_seconds);

    const std::string source_id =
        "normal_user_" + std::to_string(user_index);
    const std::string fingerprint =
        "fp_normal_" + std::to_string(user_index);
    const auto end_time = std::chrono::steady_clock::now() + duration;

    while (std::chrono::steady_clock::now() < end_time) {
        const auto interval = std::chrono::duration<double>(
            interval_distribution(generator));
        std::this_thread::sleep_for(interval);
        if (std::chrono::steady_clock::now() >= end_time) {
            break;
        }

        const std::uint64_t event_number = event_counter.fetch_add(1);
        nlohmann::json event{
            {"source_id", source_id},
            {"event_id", "evt_normal_" + std::to_string(event_number)},
            {"timestamp", static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count())},
            {"payment_token", "tok_" + std::to_string(user_index) + "_" +
                std::to_string(event_number)},
            {"fingerprint", fingerprint},
            {"payment_failed", failure_distribution(generator)}
        };

        if (!send_event(event)) {
            std::cerr << "Could not send event for "
                      << source_id
                      << std::endl;
        }
    }
}

void run_attack_bot(
    int bot_index,
    std::chrono::seconds duration,
    std::atomic<std::uint64_t>& event_counter) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<int> interval_distribution(50, 200);
    std::bernoulli_distribution failure_distribution(0.80);
    const std::string source_id =
        "attack_bot_" + std::to_string(bot_index);
    const auto end_time = std::chrono::steady_clock::now() + duration;

    while (std::chrono::steady_clock::now() < end_time) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            interval_distribution(generator)));
        if (std::chrono::steady_clock::now() >= end_time) {
            break;
        }

        const std::uint64_t event_number = event_counter.fetch_add(1);
        nlohmann::json event{
            {"source_id", source_id},
            {"event_id", "evt_attack_" + std::to_string(event_number)},
            {"timestamp", static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count())},
            {"payment_token", "tok_" + random_identifier(generator, 20)},
            {"fingerprint", "fp_" + random_identifier(generator, 12)},
            {"payment_failed", failure_distribution(generator)}
        };

        if (!send_event(event)) {
            std::cerr << "Could not send event for "
                      << source_id
                      << std::endl;
        }
    }
}

bool parse_arguments(
    int argc,
    char* argv[],
    int& duration_seconds,
    std::string& mode) {
    duration_seconds = 60;
    mode = "normal";
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (index + 1 >= argc ||
            (argument != "--duration" && argument != "--mode")) {
            return false;
        }

        const std::string value = argv[++index];
        if (argument == "--duration") {
            try {
                duration_seconds = std::stoi(value);
            } catch (const std::exception&) {
                return false;
            }
        } else if (value == "normal" ||
                   value == "attack" ||
                   value == "mixed") {
            mode = value;
        } else {
            return false;
        }
    }

    return duration_seconds > 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    int duration_seconds = 0;
    std::string mode;
    if (!parse_arguments(argc, argv, duration_seconds, mode)) {
        std::cerr << "Usage: cardshield-simulator "
                     "[--mode normal|attack|mixed] [--duration <seconds>]"
                  << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    std::atomic<std::uint64_t> event_counter{0};
    std::vector<std::thread> users;
    users.reserve(
        (mode == "mixed" ? USER_COUNT + ATTACK_BOT_COUNT :
            (mode == "attack" ? ATTACK_BOT_COUNT : USER_COUNT)));

    if (mode != "attack") {
        for (int user_index = 1;
             user_index <= USER_COUNT;
             ++user_index) {
            users.emplace_back(
                run_user,
                user_index,
                std::chrono::seconds(duration_seconds),
                std::ref(event_counter));
        }
    }

    if (mode != "normal") {
        for (int bot_index = 1;
             bot_index <= ATTACK_BOT_COUNT;
             ++bot_index) {
            users.emplace_back(
                run_attack_bot,
                bot_index,
                std::chrono::seconds(duration_seconds),
                std::ref(event_counter));
        }
    }

    for (std::thread& user : users) {
        user.join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
