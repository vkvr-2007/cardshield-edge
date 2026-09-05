#include "detection/AnomalyScorer.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {
constexpr double WINDOW_SECONDS = 60.0;
// Normalize velocity against 1.0 request per second.
constexpr double VELOCITY_THRESHOLD = 1.0;

double clamp_score(double value) {
    if (std::isnan(value)) {
        return 0.0;
    }

    return std::clamp(value, 0.0, 1.0);
}
}

ScoreResult AnomalyScorer::score(const SourceState& state) {
    ScoreResult result;

    const double request_count =
        static_cast<double>(state.timestamps.size());

    // 1. Velocity
    result.velocity_score = clamp_score(
        request_count / WINDOW_SECONDS / VELOCITY_THRESHOLD
    );

    // 2. Failure ratio
    result.failure_ratio_score =
        clamp_score(state.failure_ratio());

    // 3. Timing anomaly
    const auto& gaps = state.inter_arrival_times();

    if (gaps.size() >= 2) {
        const double mean =
            std::accumulate(gaps.begin(), gaps.end(), 0.0) /
            static_cast<double>(gaps.size());

        if (mean > 0.0) {
            double variance = 0.0;

            for (double gap : gaps) {
                const double difference = gap - mean;
                variance += difference * difference;
            }

            variance /= static_cast<double>(gaps.size());

            const double standard_deviation = std::sqrt(variance);
            const double coefficient_of_variation =
                standard_deviation / mean;

            const double timing_suspicion =
                1.0 / (1.0 + coefficient_of_variation);

            result.timing_anomaly_score =
                clamp_score(timing_suspicion * result.velocity_score);
        }
    }

    // 4. Fingerprint variation
    result.fingerprint_anomaly_score =
        clamp_score(state.distinct_fingerprint_ratio());

    // 5. Not implemented yet
    result.concentration_score = 0.0;

    // 6. Final score
    result.final_score = clamp_score(
          0.30 * result.velocity_score
        + 0.30 * result.failure_ratio_score
        + 0.20 * result.timing_anomaly_score
        + 0.15 * result.fingerprint_anomaly_score
        + 0.05 * result.concentration_score
    );

    // 7. Classification
    if (result.final_score < 0.30) {
        result.classification = "NORMAL";
    } else if (result.final_score < 0.65) {
        result.classification = "SUSPICIOUS";
    } else {
        result.classification = "ATTACK";
    }

    return result;
}