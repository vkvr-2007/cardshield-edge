#pragma once

#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"

#include <atomic>

class TcpServer {
public:
    TcpServer(
        int port,
        ThreadSafeQueue<TelemetryEvent>& event_queue,
        std::atomic<bool>& stop_flag);

    void start();

private:
    int port_;
    ThreadSafeQueue<TelemetryEvent>& event_queue_;
    std::atomic<bool>& stop_flag_;
};
