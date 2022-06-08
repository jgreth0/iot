
#include "telemetry/telemetry.hpp"
#include "presence/presence.hpp"
#include "kasa/kasa.hpp"

#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

std::mutex mtx;
std::condition_variable cv;
bool done = false, sig_io = false;

void iot() {
    std::unique_lock<std::mutex> lck(mtx);
    char name[64], addr[64];

    // Modules actively monitor and control physical devices.
    // They provide a simple abstraction for automations to interact with.
    telemetry ha = telemetry("/tmp/iot_telemetry", true);
    strcpy(name, "media");
    strcpy(addr, "10.2.0.1");
    presence media_presence = presence(name, addr);
    strcpy(name, "air filter");
    strcpy(addr, "10.72.0.2");
    kasa air_filter_plug = kasa(name, addr);

    // TODO: Place automations here.

    while (!done) {
        cv.wait(lck);
        if (sig_io) {
            ha.sync_now();
            sig_io = false;
        }
    }
}

// Terminate the "iot()" thread on interrupt.
void signalHandler(int signum) {
    std::unique_lock<std::mutex> lck(mtx);
    if (signum == SIGIO) sig_io = true;
    else done = true;
    cv.notify_all();
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            module::set_verbosity(atoi(argv[i+1]));
            i++;
        }
        else {
            printf("Options are:\n");
            printf("  -v <number> : sets the verbosiy level.\n");
            printf("    0 - OFF, only errors are reported.\n");
            printf("    1 - BASIC (default), basic startup and teardown events.\n");
            printf("    2 - LOW, adds log events (copied to stdout)\n");
            printf("    3 - MEDIUM, adds info about device thread start/stop\n");
            printf("    4 - FULL, currently same as MEDIUM\n");
            printf("    5 - DEBUG, includes function stop/start infos.\n");
            return 1;
        }
    }

    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);
    signal(SIGIO  , signalHandler);

    std::thread thread = std::thread(iot);
    thread.join();

    return 0;
}

