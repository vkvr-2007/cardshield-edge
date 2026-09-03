TCP Client
    │
        ▼
        TCP Listener
            │
                ▼
                Message Parser
                    │
                        ▼
                        Thread-Safe Queue
                            │
                                ▼
                                Worker Pool
                                    │
                                        ▼
                                        Per-Source State
                                            │
                                                ▼
                                                Behavioral Analysis
                                                    │
                                                        ▼
                                                        Anomaly Score
                                                            │
                                                                ▼
                                                                NORMAL / SUSPICIOUS / ATTACK