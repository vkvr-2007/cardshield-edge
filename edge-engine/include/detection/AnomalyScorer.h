#pragma once

#include "detection/SourceState.h"
#include <string>

struct ScoreResult {
    double velocity_score = 0.0;
    double failure_ratio_score = 0.0;
    double timing_anomaly_score = 0.0;
    double fingerprint_anomaly_score = 0.0;
    double concentration_score = 0.0;
    double final_score = 0.0;
    std::string classification;
};

class AnomalyScorer {
public:
    static ScoreResult score(const SourceState& state);
};