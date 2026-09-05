#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

struct TelemetryEvent {
    std::string source_id;
    std::string event_id;
    std::int64_t timestamp;
    std::string payment_token;
    std::string fingerprint;
    bool payment_failed;
};

inline void to_json(nlohmann::json& json, const TelemetryEvent& event) {
    json = nlohmann::json{
        {"source_id", event.source_id},
        {"event_id", event.event_id},
        {"timestamp", event.timestamp},
        {"payment_token", event.payment_token},
        {"fingerprint", event.fingerprint},
        {"payment_failed", event.payment_failed}
    };
}

inline void from_json(const nlohmann::json& json, TelemetryEvent& event) {
    event.source_id = json.at("source_id").get<std::string>();
    event.event_id = json.at("event_id").get<std::string>();
    event.timestamp = json.at("timestamp").get<std::int64_t>();
    event.payment_token = json.at("payment_token").get<std::string>();
    event.fingerprint = json.at("fingerprint").get<std::string>();
    event.payment_failed = json.at("payment_failed").get<bool>();
}

inline std::optional<TelemetryEvent> parse_line(std::string line) {
    try {
        return nlohmann::json::parse(line).get<TelemetryEvent>();
    } catch (...) {
        return std::nullopt;
    }
}