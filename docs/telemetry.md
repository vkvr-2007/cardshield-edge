# CardShield Edge Telemetry Contract

## Purpose

CardShield Edge uses synthetic telemetry events to represent payment
activity for behavioral abuse detection.

No real card numbers or sensitive payment credentials are captured,
stored, or analyzed.

## Event Format

Each event is represented as JSON:

```json
{
  "source_id": "src_001",
    "event_id": "evt_1042",
      "timestamp": 1725260000,
        "payment_token": "tok_demo_123",
          "fingerprint": "fp_07",
            "payment_failed": true
            }