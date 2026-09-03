# CardShield Edge Traffic Simulator

## Purpose

The simulator generates synthetic payment telemetry so the detection
engine can be tested without real payment traffic or card data.

## Modes

### Normal

Simulates ordinary merchant traffic.

Characteristics:

- lower request velocity
- irregular request timing
- relatively low failure ratio
- limited fingerprint variation

### Attack

Simulates automated payment-abuse/card-testing behavior.

Characteristics:

- high request velocity
- rapid or highly regular requests
- high failure ratio
- cycling synthetic payment tokens
- increased synthetic fingerprint variation

### Mixed

Generates both normal and attack traffic.

This mode is used for end-to-end demonstrations.

## Safety

The simulator must only generate synthetic identifiers.

It must never use real:

- card numbers
- payment credentials
- customer data
- authentication secrets

## Communication

The simulator sends newline-delimited JSON events to the CardShield
Edge TCP listener.

Each JSON event is one line.

Example:

{"source_id":"src_bot_01","event_id":"evt_1001","timestamp":1725260000,"payment_token":"tok_demo_01","fingerprint":"fp_07","payment_failed":true}