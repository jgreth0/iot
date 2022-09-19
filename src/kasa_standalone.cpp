
#include "kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <csignal>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
// Terminate the "interactive()" thread on interrupt.
void signalHandler(int signum) {
    fclose(stdin);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void interactive(char* name, char* addr) {
    char cmd[64];
    kasa k = kasa(name, addr);
    k.enable();
    while (1 == scanf("%63s", cmd)) {
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
            printf("Trigger sync\n");
            k.sync_now();
            printf("Done.\n");
        }
        if (!strcmp(cmd, "SYNCW" )) {
            printf("Sync and wait.\n");
            k.sync_wait();
            printf("Done.\n");
        }
        if (!strcmp(cmd, "QUERY" )) {
            printf("Query.\n");
            printf("State: %s\n", kasa::STATES[k.get_status()]);
        }
    }
    k.disable();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    char addr[64], name[64], log_file[128];
    strncpy(name, "TEST_PLUG", 64);
    strncpy(addr, "10.72.0.1", 64);
    strncpy(log_file, "kasa.log", 128);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            module::set_verbosity(atoi(argv[i+1]));
            i++;
        }
        else if (!strcmp(argv[i], "-l") && (argc > i + 1)) {
            strncpy(log_file, argv[i+1], 128);
            i++;
        }
        else if (!strcmp(argv[i], "-n") && (argc > i + 1)) {
            strncpy(name, argv[i+1], 64);
            i++;
        }
        else if (!strcmp(argv[i], "-a") && (argc > i + 1)) {
            strncpy(addr, argv[i+1], 64);
            i++;
        }
        else {
            printf("./bin/kasa_standalone is allows the user to interact with the \"kasa\" class from the command line.\n");
            printf("\n");
            printf("Commands are taken from stdin. Commands are:\n");
            printf("  ON    : Call set_target(kasa::ON)\n");
            printf("          This sets the device target state to 'ON'.\n");
            printf("  OFF   : Call set_target(kasa::OFF)\n");
            printf("          This sets the device target state to 'OFF'.\n");
            printf("  SYNC  : Call sync_now()\n");
            printf("          This triggers all device states to be checked immediately.\n");
            printf("  SYNCW : Call sync_wait()\n");
            printf("          This triggers all device states to be checked immediately.\n");
            printf("          Waits for the sync to complete before accepting another command.\n");
            printf("  QUERY : Call get_status() can convert the result to a string with kasa::STATES[]\n");
            printf("          Prints the current state.\n");
            printf("\n");
            printf("Options are:\n");
            printf("\n");
            printf("  -v <number> : sets the verbosiy level.\n");
            printf("    0 - OFF, only errors are reported.\n");
            printf("    1 - BASIC, significant events are reported once.\n");
            printf("    2 - LOW, log events are copied to stdout\n");
            printf("    3 - MEDIUM (default), reports details on every triggered event\n");
            printf("    4 - FULL, reports details on time-based events. This is not recommended\n");
            printf("        for daemons since it will report \'no change\' events multiple times\n");
            printf("        per minute.\n");
            printf("    5 - DEBUG, most function calls and returns are reported.\n");
            printf("    6 - DEBUG+, reports full network messages as they are sent and received.\n");
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
