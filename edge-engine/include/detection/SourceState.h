#pragma once

#include <cstdint>
#include <deque>
#include <string>

struct EventObservation {
    std::int64_t timestamp;
    bool payment_failed;
    std::string fingerprint;
};

struct SourceState {
    std::deque<std::int64_t> timestamps;
    std::deque<bool> failures;
    std::deque<std::string> fingerprints;
};