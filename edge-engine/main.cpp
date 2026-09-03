#include "network/TcpServer.h"
#include "queue/ThreadSafeQueue.h"
#include "telemetry/Event.h"

int main() {
    ThreadSafeQueue<TelemetryEvent> event_queue;

    TcpServer server(8080, event_queue);
    server.start();

    return 0;
}
