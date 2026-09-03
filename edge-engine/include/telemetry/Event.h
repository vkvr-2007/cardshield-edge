#pragma once

#include <cstdint>
#include <string>

struct TelemetryEvent {
    std::string source_id;
    std::string event_id;
    std::int64_t timestamp;
    std::string payment_token;
    std::string fingerprint;
    bool payment_failed;
};