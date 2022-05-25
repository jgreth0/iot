
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

void interactive() {
    char name[64], cmd[64];
    while (2 == scanf("%63s %63s", name, cmd)) {
        printf("Received command: %s\n", cmd);
        if (!strcmp(cmd, "ON" )) {
            printf("Turning on\n");
            kasa::set_target(name, kasa::ON);
            printf("Done.\n");
        }
        if (!strcmp(cmd, "OFF")) {
            printf("Turning off\n");
            kasa::set_target(name, kasa::OFF);
            printf("Done.\n");
        }
        if (!strcmp(cmd, "SYNC" )) {
            printf("Force Sync\n");
            kasa::force_sync_all();
            printf("Done.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int verbosity = 1;
    bool do_interactive = false;
    char log_file[128];
    strcpy(log_file, "kasa.log");
    char conf_file[128];
    strcpy(conf_file, "kasa.conf");

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i")) {
            do_interactive = true;
        }
        else if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            verbosity = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-l") && (argc > i + 1)) {
            strcpy(log_file, argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-c") && (argc > i + 1)) {
            strcpy(conf_file, argv[i+1]);
            i++;
        }
        else {
            printf("Options are:\n");
            printf("  -l <file name> : sets the log file.\n");
            printf("    kasa.log is the default.\n");
            printf("    Use \"\" to disable logging.\n");
            printf("  -c <file name> : sets the config file.\n");
            printf("    kasa.conf is the default.\n");
            printf("    Each line of the config file contains:\n");
            printf("      - a device name (no spaces),\n");
            printf("      - a space, and\n");
            printf("      - a device IPv4 address.\n");
            printf("  -v <number> : sets the verbosiy level.\n");
            printf("    0 - OFF, only errors are reported.\n");
            printf("    1 - BASIC (default), basic startup and teardown events.\n");
            printf("    2 - LOW, adds log events (copied to stdout)\n");
            printf("    3 - MEDIUM, adds info about device thread start/stop\n");
            printf("    4 - FULL, currently same as MEDIUM\n");
            printf("    5 - DEBUG, includes function stop/start infos.\n");
            printf("\n");
            printf("  -i : enables interactive mode\n");
            printf("    Commands are taken from stdin. Commands are:\n");
            printf("      <device name> ON  : Sets the device target state to 'ON'.\n");
            printf("      <device name> OFF : Sets the device target state to 'OFF'.\n");
            printf("      FORCE SYNC        : Triggers all device states to be checked immediately.\n");
            printf("    In non-interactive mode, device states are logged at 15 second\n");
            printf("      intervals, but there is no method to control devices.\n");
            printf("\n");
            printf("In interactive mode, the session can be terminaled with ctrl-D or ctrl-C\n");
            printf("\n");
            printf("In non-interactive mode, the session can be terminaled with ctrl-C\n");
            return 1;
        }
    }

    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);

    kasa::load_conf(log_file, conf_file, verbosity);

    if (do_interactive) {
        interactive();
    }
    else {
        std::thread thread = std::thread(loop);
        thread.join();
    }

    kasa::stop_all();

    return 0;
}
