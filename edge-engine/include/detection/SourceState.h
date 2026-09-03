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
                std::string source_id;
                    std::deque<EventObservation> recent_events;
                    };