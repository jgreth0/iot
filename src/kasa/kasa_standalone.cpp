
#include "kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <csignal>

std::mutex mtx;
std::condition_variable cv;
volatile bool done = false;

// Terminate the "loop()" thread on interrupt.
void signalHandler(int signum) {
    std::unique_lock<std::mutex> lck(mtx);
    done = true;
    cv.notify_all();
    fclose(stdin);
}

int loop() {
    std::unique_lock<std::mutex> lck(mtx);
    while (true) {
        cv.wait(lck);
        if (done) {
            break;
        }
    }
    return 0;
}

void interactive(char* name, char* addr) {
    char cmd[64];
    kasa k = kasa(name, addr);
    while (2 == scanf("%63s", cmd)) {
        printf("Received command: %s\n", cmd);
        if (!strcmp(cmd, "ON" )) {
            printf("Turning on\n");
            k.set_target(kasa::ON);
            printf("Done.\n");
        }
        if (!strcmp(cmd, "OFF")) {
            printf("Turning off\n");
            k.set_target(kasa::OFF);
            printf("Done.\n");
        }
        if (!strcmp(cmd, "SYNC" )) {
            printf("Force Sync\n");
            k.sync_now();
            printf("Done.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    char addr[64], name[64], log_file[128];
    strcpy(name, "AIR_FILTER");
    strcpy(addr, "10.72.0.2");
    strcpy(log_file, "kasa.log");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            module::set_verbosity(atoi(argv[i+1]));
            i++;
        }
        else if (!strcmp(argv[i], "-l") && (argc > i + 1)) {
            module::set_log_file(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-n") && (argc > i + 1)) {
            strcpy(name, argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-a") && (argc > i + 1)) {
            strcpy(addr, argv[i+1]);
            i++;
        }
        else {
            printf("Commands are taken from stdin. Commands are:\n");
            printf("  ON   : Sets the device target state to 'ON'.\n");
            printf("  OFF  : Sets the device target state to 'OFF'.\n");
            printf("  SYNC : Triggers all device states to be checked immediately.\n");
            printf("\n");
            printf("Options are:\n");
            printf("\n");
            printf("  -v <number> : sets the verbosiy level.\n");
            printf("    0 - OFF, only errors are reported.\n");
            printf("    1 - BASIC (default), basic startup and teardown events.\n");
            printf("    2 - LOW, adds log events (copied to stdout)\n");
            printf("    3 - MEDIUM, adds info about device thread start/stop\n");
            printf("    4 - FULL, currently same as MEDIUM\n");
            printf("    5 - DEBUG, includes function stop/start infos.\n");
            printf("\n");
            printf("  -l <file name> : log file.\n");
            printf("\n");
            printf("  -n <name> : Name (for log entries).\n");
            printf("\n");
            printf("  -a <addr> : Address of the device.\n");
            printf("\n");
            return 1;
        }
    }

    module::set_log_file(log_file);

    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);

    std::thread thread = std::thread(interactive, name, addr);
    thread.join();

    return 0;
}
