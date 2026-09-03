#pragma once

#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"

class TcpServer {
public:
    TcpServer(int port, ThreadSafeQueue<TelemetryEvent>& event_queue);

    void start();

private:
    int port_;
    ThreadSafeQueue<TelemetryEvent>& event_queue_;
};
