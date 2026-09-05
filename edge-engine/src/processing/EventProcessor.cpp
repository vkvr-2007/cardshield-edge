#include "processing/EventProcessor.h"

#include "detection/AnomalyScorer.h"

#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

EventProcessor::EventProcessor(
    ThreadSafeQueue<TelemetryEvent>& event_queue,
        std::atomic<bool>& stop_flag,
    std::size_t worker_count)
    : event_queue_(event_queue),
            stop_flag_(stop_flag),
      worker_count_(worker_count == 0 ? 1 : worker_count) {}

void EventProcessor::start() {
    workers_.reserve(worker_count_);
    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back(&EventProcessor::process_events, this);
    }

    evaluator_thread_ = std::thread(
        &EventProcessor::evaluate_periodically,
        this);
}

void EventProcessor::join() {
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    if (evaluator_thread_.joinable()) {
        evaluator_thread_.join();
    }
}

void EventProcessor::process_events() {
    while (!stop_flag_.load()) {
        std::optional<TelemetryEvent> event =
            event_queue_.wait_and_pop(stop_flag_);
        if (!event.has_value()) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);

            SourceState& state = source_states_[event->source_id];
            const std::int64_t window_size = 60;

            state.evict_older_than(event->timestamp, window_size);
            state.add_event({
                event->timestamp,
                event->payment_failed,
                event->payment_token,
                event->fingerprint
            });
            double request_velocity =
                static_cast<double>(state.timestamps.size()) / window_size;
            std::cout << "Request velocity: "
                      << request_velocity
                      << " events/sec"
                      << std::endl;
        }

        std::cout << "Worker processed event: "
                << event->event_id
                << " from "
                << event->source_id
                << std::endl;
    }
}

void EventProcessor::evaluate_periodically() {
    while (!stop_flag_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (stop_flag_.load()) {
            break;
        }

        std::vector<std::pair<std::string, SourceState>> snapshots;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            snapshots.reserve(source_states_.size());

            for (const auto& entry : source_states_) {
                snapshots.emplace_back(entry.first, entry.second);
            }
        }

        for (const auto& snapshot : snapshots) {
            const ScoreResult result = AnomalyScorer::score(snapshot.second);
            if (result.classification != "SUSPICIOUS" &&
                result.classification != "ATTACK") {
                continue;
            }

            const auto timestamp = std::chrono::duration_cast<
                std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            nlohmann::json evidence{
                {"source_id", snapshot.first},
                {"timestamp", timestamp},
                {"window_duration_seconds", 60},
                {"request_count", snapshot.second.timestamps.size()},
                {"failure_count", snapshot.second.failure_count()},
                {"failure_ratio", snapshot.second.failure_ratio()},
                {"velocity_score", result.velocity_score},
                {"timing_anomaly_score", result.timing_anomaly_score},
                {"fingerprint_anomaly_score",
                    result.fingerprint_anomaly_score},
                {"concentration_score", result.concentration_score},
                {"final_score", result.final_score},
                {"classification", result.classification}
            };
            std::cout << evidence.dump() << std::endl;
        }
    }
}