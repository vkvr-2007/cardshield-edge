#pragma once

#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"
#include "detection/SourceState.h"

#include <mutex>
#include <unordered_map>
#include <thread>

class EventProcessor {
public:
    explicit EventProcessor(ThreadSafeQueue<TelemetryEvent>& event_queue);

    void start();

private:
    void process_events();

    ThreadSafeQueue<TelemetryEvent>& event_queue_;
    std::thread worker_;
    std::unordered_map<std::string, SourceState> source_states_;
    std::mutex state_mutex_;
};