#pragma once

#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"
#include "detection/SourceState.h"

#include <atomic>
#include <mutex>
#include <cstddef>
#include <thread>
#include <unordered_map>
#include <vector>

class EventProcessor {
public:
    explicit EventProcessor(
        ThreadSafeQueue<TelemetryEvent>& event_queue,
        std::atomic<bool>& stop_flag,
        std::size_t worker_count = 3);

    void start();
    void join();

private:
    void process_events();
    void evaluate_periodically();

    ThreadSafeQueue<TelemetryEvent>& event_queue_;
    std::atomic<bool>& stop_flag_;
    std::size_t worker_count_;
    std::vector<std::thread> workers_;
    std::thread evaluator_thread_;
    std::unordered_map<std::string, SourceState> source_states_;
    std::mutex state_mutex_;
};