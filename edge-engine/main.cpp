#include "network/TcpServer.h"
#include "processing/EventProcessor.h"
#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"

int main() {
    ThreadSafeQueue<TelemetryEvent> event_queue;

    EventProcessor processor(event_queue);
    processor.start();

    TcpServer server(8080, event_queue);
    server.start();

    return 0;
}
