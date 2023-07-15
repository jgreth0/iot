
#include "modules/kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <csignal>

////////////////////////////////////////////////////////////////////////////////
// Terminate the "interactive()" thread on interrupt.
////////////////////////////////////////////////////////////////////////////////
void signalHandler(int signum) {
    fclose(stdin);
}

class kasa_testbench : public kasa {
public:
    static void interactive(kasa_testbench* k) {
        char cmd[4096];
        k->enable();
        while (1 == scanf("%4096s", cmd)) {
            k->send_recv(cmd, 4096, false);
            printf("%s\n", cmd);
        }
        k->disable();
    }
    kasa_testbench(char* name, char* addr, int cooldown = 15, int error_cooldown = 15) :
        kasa { name, addr, cooldown, error_cooldown } {
    }
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    char addr[64], name[64], log_file[128];
    strncpy(name, "TEST_PLUG", 64);
    strncpy(addr, "10.72.0.1", 64);
    strncpy(log_file, "kasa.log", 128);
    module::set_verbosity(0);
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
            printf("./bin/kasa_testbench allows the user to interact directly with a kasa device.\n");
            printf("Commands taken from stdin are sent directly to the device. Responses are printed to stdout.\n");
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

    kasa_testbench k = kasa_testbench(name, addr);
    std::thread thread = std::thread(kasa_testbench::interactive, &k);
    thread.join();

    return 0;
}
