# CardShield Edge TCP Protocol

## Version 1

CardShield Edge uses a simple line-delimited JSON protocol.

### Message Boundary

One JSON event equals one line.

Example:

{"source_id":"src_001","event_id":"evt_1042","timestamp":1725260000,"payment_token":"tok_demo_123","fingerprint":"fp_07","payment_failed":true}

The message is terminated by a newline character:

\n

## Processing Flow

TCP client
    ↓
    TCP listener
        ↓
        Read until newline
            ↓
            Parse JSON event
                ↓
                Place event into thread-safe queue
                    ↓
                    Worker thread processes event

                    ## Design Goal

                    The protocol is intentionally simple for the first version so that
                    networking, concurrency, and behavioral analysis can be developed and
                    tested independently.