# CardShield Edge Detection Model

## Goal

Detect behavioral patterns associated with synthetic payment abuse
without analyzing real card numbers.

## Behavioral Signals

CardShield Edge evaluates:

1. Request velocity
2. Payment failure ratio
3. Inter-request timing
4. Synthetic fingerprint variation
5. Source concentration

## Anomaly Score

Version 1 uses the following weighted score:

score =
    0.30 * velocity
      + 0.30 * failure_ratio
        + 0.20 * timing
          + 0.15 * fingerprint
            + 0.05 * concentration

            ## Risk Thresholds

            | Score | Classification |
            |---|---|
            | < 0.30 | NORMAL |
            | 0.30–< 0.65 | SUSPICIOUS |
            | >= 0.65 | ATTACK |

            These are initial demonstration thresholds and are not claimed to be
            scientifically tuned.

            ## Detection Principle

            The detector uses behavioral telemetry rather than card-number
            analysis.

            Real card numbers must never be captured, stored, transmitted, or
            analyzed.