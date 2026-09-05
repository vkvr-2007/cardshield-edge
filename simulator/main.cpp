#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
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

struct DatasetCategory {
    const char* name;
    int count;
    bool attack_label;
};

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

nlohmann::json make_dataset_example(
    const DatasetCategory& category,
    int example_index,
    std::mt19937& generator) {
    std::uniform_int_distribution<int> request_count_distribution;
    std::uniform_int_distribution<int> interval_distribution;
    double failure_probability = 0.05;
    int distinct_fingerprint_count = 1;
    int distinct_token_count = 1;

    if (std::string(category.name) == "normal") {
        request_count_distribution = std::uniform_int_distribution<int>(
            8,
            20);
        interval_distribution = std::uniform_int_distribution<int>(
            2500,
            8000);
        distinct_token_count = 8;
    } else if (std::string(category.name) == "legitimate_high_volume") {
        request_count_distribution = std::uniform_int_distribution<int>(
            40,
            70);
        interval_distribution = std::uniform_int_distribution<int>(
            700,
            1800);
        distinct_token_count = 40;
    } else if (std::string(category.name) == "card_testing_bot") {
        request_count_distribution = std::uniform_int_distribution<int>(
            60,
            100);
        interval_distribution = std::uniform_int_distribution<int>(
            50,
            200);
        failure_probability = 0.80;
        distinct_fingerprint_count = 60;
        distinct_token_count = 90;
    } else if (std::string(category.name) == "slow_high_failure") {
        request_count_distribution = std::uniform_int_distribution<int>(
            5,
            12);
        interval_distribution = std::uniform_int_distribution<int>(
            8000,
            15000);
        failure_probability = 0.80;
        distinct_token_count = 5;
    } else {
        request_count_distribution = std::uniform_int_distribution<int>(
            60,
            100);
        interval_distribution = std::uniform_int_distribution<int>(
            100,
            100);
        distinct_fingerprint_count = 2;
        distinct_token_count = 80;
    }

    const int request_count = request_count_distribution(generator);
    std::binomial_distribution<int> failure_distribution(
        request_count,
        failure_probability);
    std::vector<double> inter_arrival_times;
    inter_arrival_times.reserve(request_count > 0 ? request_count - 1 : 0);

    for (int index = 1; index < request_count; ++index) {
        inter_arrival_times.push_back(
            static_cast<double>(interval_distribution(generator)));
    }

    const int failure_count = failure_distribution(generator);
    const double failure_ratio = request_count == 0
        ? 0.0
        : static_cast<double>(failure_count) / request_count;
    const double distinct_token_ratio =
        static_cast<double>(std::min(distinct_token_count, request_count)) /
        request_count;
    const double distinct_fingerprint_ratio =
        static_cast<double>(std::min(
            distinct_fingerprint_count,
            request_count)) / request_count;

    return nlohmann::json{
        {"source_id", std::string(category.name) + "_" +
            std::to_string(example_index)},
        {"request_count", request_count},
        {"failure_count", failure_count},
        {"failure_ratio", failure_ratio},
        {"distinct_token_ratio", distinct_token_ratio},
        {"distinct_fingerprint_ratio", distinct_fingerprint_ratio},
        {"inter_arrival_times", inter_arrival_times},
        {"true_label", category.attack_label ? "ATTACK" : "NORMAL"}
    };
}

bool generate_dataset() {
    const std::filesystem::path output_path =
        std::filesystem::path("dataset") / "full_dataset.jsonl";
    std::filesystem::create_directories(output_path.parent_path());

    std::ofstream output(output_path);
    if (!output) {
        std::cerr << "Could not open dataset output: "
                  << output_path.string()
                  << std::endl;
        return false;
    }

    std::random_device random_device;
    std::mt19937 generator(random_device());
    const DatasetCategory categories[] = {
        {"normal", 30, false},
        {"legitimate_high_volume", 10, false},
        {"card_testing_bot", 20, true},
        {"slow_high_failure", 10, false},
        {"regular_low_failure_bot", 10, true}
    };

    for (const DatasetCategory& category : categories) {
        for (int index = 1; index <= category.count; ++index) {
            output << make_dataset_example(
                category,
                index,
                generator).dump()
                   << '\n';
        }
    }

    std::cout << "Wrote dataset: "
              << output_path.string()
              << " (80 examples)"
              << std::endl;
    return true;
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
                   value == "mixed" ||
                   value == "dataset-gen") {
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
                     "[--mode normal|attack|mixed|dataset-gen] "
                     "[--duration <seconds>]"
                  << std::endl;
        return 1;
    }

    if (mode == "dataset-gen") {
        return generate_dataset() ? 0 : 1;
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
