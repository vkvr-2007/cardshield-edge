#include "network/TcpServer.h"
#include "processing/EventProcessor.h"
#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"

#include <atomic>
#include <csignal>

namespace {
std::atomic<bool>* active_stop_flag = nullptr;

void handle_sigint(int) {
    if (active_stop_flag != nullptr) {
        active_stop_flag->store(true);
    }
}
}

int main() {
    std::atomic<bool> stop_flag{false};
    active_stop_flag = &stop_flag;
    std::signal(SIGINT, handle_sigint);

    ThreadSafeQueue<TelemetryEvent> event_queue;

    EventProcessor processor(event_queue, stop_flag);
    processor.start();

    TcpServer server(8080, event_queue, stop_flag);
    server.start();
    processor.join();

    return 0;
}
