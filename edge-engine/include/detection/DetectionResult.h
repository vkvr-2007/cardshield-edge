#pragma once

#include <string>

enum class RiskLevel {
    NORMAL,
        SUSPICIOUS,
            ATTACK
            };

            struct DetectionResult {
                std::string source_id;
                    RiskLevel risk_level;
                        double anomaly_score;

                            double velocity_score;
                                double failure_ratio_score;
                                    double timing_score;
                                        double fingerprint_score;
                                            double concentration_score;
                                            };