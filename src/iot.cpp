
#include "modules/signal_handler.hpp"
#include "automations/air_filter.hpp"
#include "automations/switch_plug.hpp"
#include "automations/outside_lights.hpp"

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

    // Monitor only
    kasa shed_lights = kasa("light_shed", "10.4.1.8", 5);
    shed_lights.enable();
    kasa garage_lights = kasa("light_garage", "10.4.3.1", 5);
    garage_lights.enable();
    kasa freezer = kasa("freezer", "10.4.2.2", 5);
    freezer.enable();
    kasa car = kasa("car", "10.4.2.3", 5);
    car.enable();
    // kasa dehumidifier = kasa("dehumidifier", "10.4.2.5", 5);
    // dehumidifier.enable();
    kasa tv_plug = kasa("tv_plug", "10.4.2.6", 5);
    tv_plug.enable();
    // kasa plug_pm_7 = kasa("plug_pm_7", "10.4.2.7", 5);
    // plug_pm_7.enable();
    // kasa plug_pm_8 = kasa("plug_pm_8", "10.4.2.8", 5);
    // plug_pm_8.enable();

    // Air filter control
    kasa air_filter_plug = kasa("air_filter", "10.4.0.2", 5);
    air_filter_plug.enable();

    // Switch+plug
    kasa kasa_bed_switch = kasa("bed_switch", "10.4.1.9", 1);
    kasa_bed_switch.enable();
    kasa kasa_bed_plug_low = kasa("bed_plug_low", "10.4.0.1", 5);
    kasa_bed_plug_low.enable();
    kasa kasa_bed_plug_high = kasa("bed_plug_high", "10.4.5.1", 5);
    kasa_bed_plug_high.enable();
    kasa kasa_office_switch = kasa("office_switch", "10.4.1.2", 1);
    kasa_office_switch.enable();
    kasa kasa_office_plug = kasa("office_plug", "10.4.2.1", 5);
    kasa_office_plug.enable();

    // Outside lights
    const int front_ct = 3;
    kasa front_lights[front_ct] = {
        kasa("light_tree_xmas", "10.4.2.4", 5),
        kasa("light_front_porch", "10.4.1.1", 5),
        kasa("light_front_pole", "10.4.3.2", 5)};
    for (int i = 0; i < front_ct; i++) front_lights[i].enable();
    const int rear_ct = 5;
    kasa rear_lights[rear_ct] = {
        kasa("light_rear_garage", "10.4.1.4", 5),
        kasa("light_rear_deck", "10.4.1.5", 5),
        kasa("light_rear_flood", "10.4.1.6", 5),
        kasa("light_rear_basement", "10.4.1.7", 5),
        kasa("light_front_garage", "10.4.1.3", 5)};
    for (int i = 0; i < rear_ct; i++) rear_lights[i].enable();

    // Automations use modules to acheive high-level objectives.

    air_filter af = air_filter(&air_filter_plug);
    af.enable();

    switch_plug sp = switch_plug(&kasa_bed_switch, &kasa_bed_plug_low, &kasa_bed_plug_high,
        &kasa_office_switch, &kasa_office_plug);
    sp.enable();

    outside_lights ol = outside_lights(front_lights, front_ct, rear_lights, rear_ct);
    ol.enable();

    while (!done) cv.wait(lck);

    // Automations
    ol.disable();
    sp.disable();
    af.disable();

    // Outside lights
    for (int i = 0; i < rear_ct; i++) rear_lights[i].disable();
    for (int i = 0; i < front_ct; i++) front_lights[i].disable();

    // Switch+plug
    kasa_bed_plug_high.disable();
    kasa_bed_plug_low.disable();
    kasa_bed_switch.disable();
    kasa_office_switch.disable();
    kasa_office_plug.disable();

    // Air filter control
    air_filter_plug.disable();

    // Monitor only
    shed_lights.disable();
    garage_lights.disable();
    freezer.disable();
    car.disable();
    // dehumidifier.disable();
    tv_plug.disable();
    // plug_pm_7.disable();
    // plug_pm_8.disable();
}

////////////////////////////////////////////////////////////////////////////////
// Terminate the "iot()" thread on interrupt.
////////////////////////////////////////////////////////////////////////////////
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

