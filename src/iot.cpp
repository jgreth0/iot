
#include "shmem.hpp"
#include "presence.hpp"
#include "presence_icmp.hpp"
#include "kasa.hpp"
#include "air_filter.hpp"
#include "signal_handler.hpp"

#include <execinfo.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
std::mutex mtx;
std::condition_variable cv;
bool done = false;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void iot() {
    std::unique_lock<std::mutex> lck(mtx);
    char name[64], addr[64];

    // Modules actively monitor and control physical devices.
    // They provide a simple abstraction for automations to interact with.
    // telemetry ha = telemetry("/tmp/iot_telemetry", true);

    signal_handler sigio_handler = signal_handler(SIGIO);

    strncpy(name, "media", 64);
    strncpy(addr, "10.2.0.3", 64);
    presence_icmp media_presence = presence_icmp(name, addr);
    media_presence.enable();


    strncpy(name, "air filter", 64);
    strncpy(addr, "10.4.0.2", 64);
    kasa air_filter_plug = kasa(name, addr);
    air_filter_plug.enable();

    // Automations use modules to acheive high-level objectives.

    air_filter af = air_filter(&media_presence, &air_filter_plug);
    af.enable();

    while (!done) cv.wait(lck);

    af.disable();
    air_filter_plug.disable();
    media_presence.disable();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
// Terminate the "iot()" thread on interrupt.
void signalHandler(int signum) {
    if (signum == SIGSEGV) {
        void *array[50];
        size_t size;

        // get void*'s for all entries on the stack
        size = backtrace(array, 50);

        // print out all the frames to stderr
        fprintf(stderr, "Error: signal %d:\n", signum);
        backtrace_symbols_fd(array, size, STDOUT_FILENO);
        exit(-1);
    }
    std::unique_lock<std::mutex> lck(mtx);
    done = true;
    cv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    char log_file[128];
    strncpy(log_file, "iot.log", 128);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            module::set_verbosity(atoi(argv[i+1]));
            i++;
        }
        else if (!strcmp(argv[i], "-l") && (argc > i + 1)) {
            strncpy(log_file, argv[i+1], 128);
            i++;
        }
        else {
            printf("Options are:\n");
            printf("  -v <number> : sets the verbosiy level.\n");
            printf("    0 - OFF, only errors are reported.\n");
            printf("    1 - BASIC (default), basic startup and teardown events.\n");
            printf("    2 - LOW, adds log events (copied to stdout)\n");
            printf("    3 - MEDIUM, adds info about device thread start/stop\n");
            printf("    4 - FULL, adds a few interesting events\n");
            printf("    5 - DEBUG, adds function stop/start infos.\n");
            printf("    6 - DEBUG+, adds network messages.\n");
            printf("\n");
            printf("  -l <file name> : log file.\n");
            return 1;
        }
    }

    module::set_log_file(log_file);

    signal(SIGSEGV, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);

    std::thread thread = std::thread(iot);
    thread.join();

    return 0;
}

