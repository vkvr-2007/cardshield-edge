#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

struct EventObservation {
    std::int64_t timestamp;
    bool payment_failed;
    std::string payment_token;
    std::string fingerprint;
};

struct SourceState {
    std::deque<std::int64_t> timestamps;
    std::deque<bool> failures;
    std::deque<std::string> payment_tokens;
    std::deque<std::string> fingerprints;

    void add_event(const EventObservation& observation) {
        timestamps.push_back(observation.timestamp);
        failures.push_back(observation.payment_failed);
        payment_tokens.push_back(observation.payment_token);
        fingerprints.push_back(observation.fingerprint);
    }

    void evict_older_than(
        std::int64_t timestamp,
        std::int64_t window_size) {
        std::size_t index = 0;
        while (index < timestamps.size()) {
            if (timestamp - timestamps[index] > window_size) {
                timestamps.erase(timestamps.begin() + index);
                failures.erase(failures.begin() + index);
                payment_tokens.erase(payment_tokens.begin() + index);
                fingerprints.erase(fingerprints.begin() + index);
            } else {
                ++index;
            }
        }
    }

    std::size_t failure_count() const {
        std::size_t count = 0;
        for (bool failure : failures) {
            if (failure) {
                ++count;
            }
        }
        return count;
    }

    double failure_ratio() const {
        if (timestamps.empty()) {
            return 0.0;
        }

        return static_cast<double>(failure_count()) / timestamps.size();
    }

    double distinct_token_ratio() const {
        if (timestamps.empty()) {
            return 0.0;
        }

        return static_cast<double>(distinct_token_count()) / timestamps.size();
    }

    double distinct_fingerprint_ratio() const {
        if (timestamps.empty()) {
            return 0.0;
        }

        return static_cast<double>(distinct_fingerprint_count()) /
            timestamps.size();
    }

    std::vector<double> inter_arrival_times() const {
        std::vector<std::int64_t> ordered_timestamps(
            timestamps.begin(),
            timestamps.end());
        std::sort(
            ordered_timestamps.begin(),
            ordered_timestamps.end());

        std::vector<double> gaps;
        if (ordered_timestamps.size() < 2) {
            return gaps;
        }

        gaps.reserve(ordered_timestamps.size() - 1);
        for (std::size_t index = 1;
             index < ordered_timestamps.size();
             ++index) {
            gaps.push_back(static_cast<double>(
                ordered_timestamps[index] -
                ordered_timestamps[index - 1]) * 1000.0);
        }

        return gaps;
    }

private:
    std::size_t distinct_token_count() const {
        return std::unordered_set<std::string>(
            payment_tokens.begin(), payment_tokens.end()).size();
    }

    std::size_t distinct_fingerprint_count() const {
        return std::unordered_set<std::string>(
            fingerprints.begin(), fingerprints.end()).size();
    }
};