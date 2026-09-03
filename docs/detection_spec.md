# CardShield Edge Detection Specification

## Purpose

The detection engine identifies behavioral patterns associated with
synthetic payment abuse.

The detector does not inspect real card numbers or payment credentials.

## Detection Window

The engine maintains a sliding window of recent events for each
synthetic `source_id`.

The window is used to calculate behavioral features.

## Behavioral Features

### 1. Request Velocity

Measures how many events a source generates within the active window.

Higher event frequency produces a higher velocity score.

### 2. Failure Ratio

Measures the proportion of payment attempts that fail within the
active window.

A high failure ratio can indicate automated payment testing.

### 3. Inter-Request Timing

Measures the timing pattern between consecutive events.

Highly regular or unusually rapid activity can increase the timing
anomaly score.

### 4. Fingerprint Variation

Measures variation in synthetic client fingerprints associated with
the same source.

Rapid fingerprint changes can indicate automated or evasive behavior.

### 5. Source Concentration

Measures how concentrated suspicious activity is around a particular
source during the observation window.

## Anomaly Score

The initial detector uses:

score =
    0.30 * velocity_score
  + 0.30 * failure_ratio_score
  + 0.20 * timing_score
  + 0.15 * fingerprint_score
  + 0.05 * concentration_score

Each component is normalized to the range 0.0–1.0.

The final anomaly score is also expected to remain in the range 0.0–1.0.

## Classification

| Anomaly Score | Classification |
|---|---|
| < 0.30 | NORMAL |
| 0.30–< 0.65 | SUSPICIOUS |
| >= 0.65 | ATTACK |

These thresholds are initial v1 demonstration thresholds and are not
claimed to be scientifically optimized.

## Detection Output

Every analyzed source should produce:

- source ID
- risk classification
- overall anomaly score
- velocity score
- failure ratio score
- timing score
- fingerprint variation score
- concentration score

The structured output will later be consumed by the AI analyst.

## Security Boundary

The detection system uses behavioral telemetry only.

It must never:

- capture real card numbers
- store real card numbers
- transmit real card numbers
- analyze real card numbers
- require real payment credentials

All payment identifiers used by the simulator are synthetic.
