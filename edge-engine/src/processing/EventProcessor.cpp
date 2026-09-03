#include "processing/EventProcessor.h"

#include <iostream>

EventProcessor::EventProcessor(
    ThreadSafeQueue<TelemetryEvent>& event_queue)
    : event_queue_(event_queue) {}

void EventProcessor::start() {
    worker_ = std::thread(&EventProcessor::process_events, this);
}

void EventProcessor::process_events() {
    while (true) {
        TelemetryEvent event = event_queue_.wait_and_pop();

        {
            std::lock_guard<std::mutex> lock(state_mutex_);

            SourceState& state = source_states_[event.source_id];
            const std::int64_t window_size = 60;

            while (!state.timestamps.empty() &&
                event.timestamp - state.timestamps.front() > window_size) {
                state.timestamps.pop_front();
                state.failures.pop_front();
                state.fingerprints.pop_front();
}

            state.timestamps.push_back(event.timestamp);
            state.failures.push_back(event.payment_failed);
            state.fingerprints.push_back(event.fingerprint);
        }

        std::cout << "Worker processed event: "
                << event.event_id
                << " from "
                << event.source_id
                << std::endl;
    }
}