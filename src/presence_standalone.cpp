
#include "presence_tcp.hpp"
#include "presence_icmp.hpp"
#include <csignal>
#include <cstring>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
// Terminate the "interactive()" thread on interrupt.
void signalHandler(int signum) {
    printf("SIGNAL RECEIVED: %d\n", signum);
    fclose(stdin);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void interactive(char* name, char* addr, int port) {
    char cmd[64];
    presence_tcp  p_tcp  = presence_tcp (name, addr, port);
    presence_icmp p_icmp = presence_icmp(name, addr);
    p_tcp.enable();
    p_icmp.enable();
    while (1 == scanf("%63s", cmd)) {
        if (!strcmp(cmd, "SYNC" )) {
            printf("Trigger sync\n");
            p_tcp.sync_now();
            p_icmp.sync_now();
            printf("Done.\n");
        }
        if (!strcmp(cmd, "SYNCW" )) {
            printf("Sync and wait.\n");
            p_tcp.sync_wait();
            p_icmp.sync_wait();
            printf("Done.\n");
        }
        if (!strcmp(cmd, "QUERY" )) {
            printf("Query.\n");
            printf("TCP: %d; ICMP: %d\n", p_tcp.present(), p_icmp.present());
        }
    }
    p_icmp.disable();
    p_tcp.disable();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    char addr[64], name[64], log_file[128];
    int port = 9999;
    strncpy(name, "TEST_PLUG", 64);
    strncpy(addr, "10.72.0.1", 64);
    strncpy(log_file, "presence.log", 128);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") && (argc > i + 1)) {
            module::set_verbosity(atoi(argv[i+1]));
            i++;
        }
        else if (!strcmp(argv[i], "-p") && (argc > i + 1)) {
            port = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-l") && (argc > i + 1)) {
            module::set_log_file(argv[i+1]);
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
            printf("Commands are taken from stdin. Commands are:\n");
            printf("  SYNC  : Triggers all device states to be checked immediately.\n");
            printf("  SYNCW : Triggers all device states to be checked immediately.\n");
            printf("          Waits for the sync to complete before accepting another command.\n");
            printf("  QUERY : Prints the current state.\n");
            printf("\n");
            printf("Options are:\n");
            printf("\n");
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
            printf("\n");
            printf("  -n <name> : Name (for log entries).\n");
            printf("\n");
            printf("  -a <addr> : Address of the device.\n");
            printf("\n");
            printf("  -p <port> : An open TCP port on the device.\n");
            printf("\n");
            return 1;
        }
    }

    module::set_log_file(log_file);

    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);

    std::thread thread = std::thread(interactive, name, addr, port);
    thread.join();

    return 0;
}
