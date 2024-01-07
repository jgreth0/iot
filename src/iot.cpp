
#include "modules/signal_handler.hpp"
#include "modules/presence_icmp.hpp"
#include "automations/kasa_alarm.hpp"
#include "automations/kasa_timer.hpp"
#include "automations/kasa_dimmer.hpp"
#include "automations/switch_plug.hpp"
#include "automations/presence_ctrl.hpp"
#include "automations/automation_module.hpp"
#include "automations/automation_group.hpp"
#include "automations/kasa_conditional_automation.hpp"
#include "automations/time_conditional_automation.hpp"
#include "automations/state_matcher.hpp"

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

    automation_module am = automation_module();
    am.enable();
    sun_time_fetcher sstf = sun_time_fetcher(true);
    sstf.enable();
    sun_time_fetcher srtf = sun_time_fetcher(false);
    srtf.enable();

    // Modules actively monitor and control physical devices.
    // They provide a simple abstraction for automations to interact with.

    signal_handler sigio_handler = signal_handler(SIGIO);

    // Monitor only
    kasa shed_lights = kasa("light_shed", "10.4.1.8", 5);
    shed_lights.enable();
    kasa garage_lights = kasa("light_garage", "10.4.3.1", 5);
    garage_lights.enable();
    kasa freezer_plug = kasa("freezer", "10.4.2.2", 5);
    freezer_plug.enable();
    kasa car_plug = kasa("car", "10.4.2.3", 5);
    car_plug.enable();
    kasa dehumidifier_plug = kasa("dehumidifier", "10.4.2.5", 5);
    dehumidifier_plug.enable();
    kasa attic_fan = kasa("attic_fan", "10.4.1.10", 5);
    attic_fan.enable();

    // vegetable light
    kasa vegetable_light = kasa("vegetable_light", "10.4.2.6", 5);
    vegetable_light.enable();
    vegetable_light.listen(&am);

    // Bose system on a smart plug
    kasa bose_plug = kasa("bose", "10.4.2.1", 5);
    bose_plug.enable();
    bose_plug.listen(&am);

    // Subwoofer control
    kasa subwoofer_plug = kasa("subwoofer", "10.4.0.1", 5);
    subwoofer_plug.enable();
    subwoofer_plug.listen(&am);
    presence_icmp tv_presence = presence_icmp("TV", "10.7.0.3", 300);
    tv_presence.enable();
    tv_presence.listen(&am);

    // Air filter control
    kasa air_filter_plug = kasa("air_filter", "10.4.2.8", 5);
    air_filter_plug.enable();
    air_filter_plug.listen(&am);

    // Switch+plug
    kasa kasa_bed_switch = kasa("bed_switch", "10.4.1.9", 1);
    kasa_bed_switch.enable();
    kasa_bed_switch.listen(&am);
    kasa kasa_bed_plug_low = kasa("bed_plug_low", "10.4.2.7", 5);
    kasa_bed_plug_low.enable();
    kasa_bed_plug_low.listen(&am);
    //kasa kasa_bed_plug_high = kasa("bed_plug_high", "10.4.5.1", 5);
    kasa kasa_bed_plug_high = kasa("bed_plug_high", "10.4.2.12", 5);
    kasa_bed_plug_high.enable();
    kasa_bed_plug_high.listen(&am);
    kasa kasa_office_switch = kasa("office_switch", "10.4.1.2", 1);
    kasa_office_switch.enable();
    kasa_office_switch.listen(&am);
    kasa kasa_office_plug = kasa("office_plug", "10.4.0.2", 5);
    kasa_office_plug.enable();
    kasa_office_plug.listen(&am);

    // Outside lights - Front
    kasa light_tree_xmas     = kasa("light_tree_xmas    ", "10.4.2.4", 5);
    light_tree_xmas.enable();
    light_tree_xmas.listen(&am);
    kasa light_front_porch   = kasa("light_front_porch  ", "10.4.1.1", 5);
    light_front_porch.enable();
    light_front_porch.listen(&am);
    kasa light_front_garage  = kasa("light_front_garage ", "10.4.1.3", 5);
    light_front_garage.enable();
    light_front_garage.listen(&am);

    // Outside lights - Rear
    kasa light_rear_garage   = kasa("light_rear_garage  ", "10.4.1.4", 5);
    light_rear_garage.enable();
    light_rear_garage.listen(&am);
    kasa light_rear_deck     = kasa("light_rear_deck    ", "10.4.1.5", 5);
    light_rear_deck.enable();
    light_rear_deck.listen(&am);
    kasa light_rear_flood    = kasa("light_rear_flood   ", "10.4.1.6", 5);
    light_rear_flood.enable();
    light_rear_flood.listen(&am);
    kasa light_rear_basement = kasa("light_rear_basement", "10.4.1.7", 5);
    light_rear_basement.enable();
    light_rear_basement.listen(&am);
    kasa light_front_pole    = kasa("light_front_pole   ", "10.4.3.2", 5);
    light_front_pole.enable();
    light_front_pole.listen(&am);

    // Automations use modules to acheive high-level objectives.

    switch_plug spl = switch_plug("bed low ", &kasa_bed_switch   , &kasa_bed_plug_low );
    am.add_automation(&spl);
    switch_plug spo = switch_plug("office"  , &kasa_office_switch, &kasa_office_plug  );
    am.add_automation(&spo);

    // High light follows the switch if it is on or if it is day time
    switch_plug sph = switch_plug("bed high", &kasa_bed_switch   , &kasa_bed_plug_high);
    kasa_conditional_automation sph_on = kasa_conditional_automation("bed high on", &sph, kasa::ON, kasa_conditional_automation::OR, 0);
    sph_on.add_plug(&kasa_bed_plug_high);
    am.add_automation(&sph_on);
    time_conditional_automation sph_before = time_conditional_automation("bed high before", &sph       , time_conditional_automation::BEFORE, 21, 0);
    time_conditional_automation sph_after  = time_conditional_automation("bed high after ", &sph_before, time_conditional_automation::AFTER , 7, 30);
    am.add_automation(&sph_after);

    kasa_alarm bed_high_evening = kasa_alarm(&kasa_bed_plug_high, "bed high evening", kasa::OFF, 21, 0);
    am.add_automation(&bed_high_evening);

    kasa_alarm bed_morning = kasa_alarm(&kasa_bed_switch, "bed morning", kasa::ON, 7, 30);
    am.add_automation(&bed_morning);
    kasa_alarm bed_evening = kasa_alarm(&kasa_bed_switch, "bed evening", kasa::ON, 21, 0);
    am.add_automation(&bed_evening);
    kasa_timer bed_timer = kasa_timer(&kasa_bed_switch, "bed_timer", kasa::OFF, 4, 0);
    am.add_automation(&bed_timer);

    // Set dimmer states
    // kasa_dimmer dimmer_rise = kasa_dimmer(&kasa_bed_plug_high, "rise", 1, 35, 7, 30, 900);
    // time_conditional_automation dimmer_rise_before = time_conditional_automation(
    //     "dimmer rise before", &dimmer_rise, time_conditional_automation::BEFORE, 7, 30);
    // am.add_automation(&dimmer_rise_before);
    // kasa_dimmer dimmer_hold = kasa_dimmer(&kasa_bed_plug_high, "hold", 100, 100, 7, 45, 0);
    // time_conditional_automation dimmer_hold_after  = time_conditional_automation(
    //     "dimmer hold after ", &dimmer_hold      , time_conditional_automation::AFTER ,  7, 45);
    // time_conditional_automation dimmer_hold_before = time_conditional_automation(
    //     "dimmer hold before", &dimmer_hold_after, time_conditional_automation::BEFORE, 20, 45);
    // am.add_automation(&dimmer_hold_before);
    // kasa_dimmer dimmer_fall = kasa_dimmer(&kasa_bed_plug_high, "fall", 35, 1, 20, 45, 900);
    // time_conditional_automation dimmer_fall_after  = time_conditional_automation(
    //     "dimmer fall after ", &dimmer_fall      , time_conditional_automation::AFTER ,  20, 45);
    // am.add_automation(&dimmer_fall_after);

    presence_ctrl sw = presence_ctrl("subwoofer", &subwoofer_plug, &tv_presence);
    am.add_automation(&sw);

    kasa_alarm af_on  = kasa_alarm(&air_filter_plug, "AF ON ", kasa::ON , 5, 30, 60);
    am.add_automation(&af_on);
    kasa_alarm af_off = kasa_alarm(&air_filter_plug, "AF OFF", kasa::OFF, 6, 30);
    am.add_automation(&af_off);

    kasa_timer vegetable_light_timer = kasa_timer(&vegetable_light, "vegetable_light_timer", kasa::OFF, 16, 0);
    am.add_automation(&vegetable_light_timer);
    kasa_alarm morning_vegetable_light     = kasa_alarm(&vegetable_light    , "morning_vegetable_light    ", kasa::ON, 5, 0);
    am.add_automation(&morning_vegetable_light);

    kasa_alarm morning_light_tree_xmas     = kasa_alarm(&light_tree_xmas    , "morning_light_tree_xmas    ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_tree_xmas);
    kasa_alarm morning_light_front_porch   = kasa_alarm(&light_front_porch  , "morning_light_front_porch  ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_front_porch);
    kasa_alarm morning_light_front_garage  = kasa_alarm(&light_front_garage , "morning_light_front_garage ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_front_garage);
    kasa_alarm morning_light_rear_garage   = kasa_alarm(&light_rear_garage  , "morning_light_rear_garage  ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_rear_garage);
    kasa_alarm morning_light_rear_deck     = kasa_alarm(&light_rear_deck    , "morning_light_rear_deck    ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_rear_deck);
    kasa_alarm morning_light_rear_flood    = kasa_alarm(&light_rear_flood   , "morning_light_rear_flood   ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_rear_flood);
    kasa_alarm morning_light_rear_basement = kasa_alarm(&light_rear_basement, "morning_light_rear_basement", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_rear_basement);
    kasa_alarm morning_light_front_pole    = kasa_alarm(&light_front_pole   , "morning_light_front_pole   ", kasa::OFF, 8, 0, 0, &srtf);
    am.add_automation(&morning_light_front_pole);

    state_matcher osm = state_matcher("outdoor lights");
    osm.add_plug(&light_rear_garage);
    osm.add_plug(&light_rear_deck);
    osm.add_plug(&light_rear_flood);
    osm.add_plug(&light_rear_basement);
    osm.add_plug(&light_front_pole);
    am.add_automation(&osm);

    kasa_alarm evening_light_tree_xmas     = kasa_alarm(&light_tree_xmas    , "evening_light_tree_xmas    ", kasa::ON, 15, 0, 60, &sstf);
    am.add_automation(&evening_light_tree_xmas);
    kasa_alarm evening_light_front_porch   = kasa_alarm(&light_front_porch  , "evening_light_front_porch  ", kasa::ON, 15, 0, 60, &sstf);
    am.add_automation(&evening_light_front_porch);
    kasa_alarm evening_light_front_garage  = kasa_alarm(&light_front_garage , "evening_light_front_garage ", kasa::ON, 15, 0, 60, &sstf);
    am.add_automation(&evening_light_front_garage);

    automation_group night_lights = automation_group("night lights");
    kasa_alarm night_light_tree_xmas     = kasa_alarm(&light_tree_xmas    , "night_light_tree_xmas    ", kasa::OFF, 22, 15);
    night_lights.add_automation(&night_light_tree_xmas);
    kasa_alarm night_light_front_porch   = kasa_alarm(&light_front_porch  , "night_light_front_porch  ", kasa::OFF, 22, 15);
    night_lights.add_automation(&night_light_front_porch);
    kasa_alarm night_light_front_garage  = kasa_alarm(&light_front_garage , "night_light_front_garage ", kasa::OFF, 22, 15);
    night_lights.add_automation(&night_light_front_garage);
    kasa_conditional_automation night_light_condition = kasa_conditional_automation("night lights", &night_lights, kasa::OFF, kasa_conditional_automation::AND, 15);
    night_light_condition.add_plug(&light_rear_garage);
    night_light_condition.add_plug(&light_rear_deck);
    night_light_condition.add_plug(&light_rear_flood);
    night_light_condition.add_plug(&light_rear_basement);
    night_light_condition.add_plug(&light_front_pole);
    am.add_automation(&night_light_condition);

    while (!done) cv.wait(lck);

    // Automations
    am.disable();
    sstf.disable();
    srtf.disable();

    // Outside lights
    light_tree_xmas.disable();
    light_front_porch.disable();
    light_front_garage.disable();
    light_rear_garage.disable();
    light_rear_deck.disable();
    light_rear_flood.disable();
    light_rear_basement.disable();
    light_front_pole.disable();

    // Switch+plug
    kasa_bed_plug_high.disable();
    kasa_bed_plug_low.disable();
    kasa_bed_switch.disable();
    kasa_office_switch.disable();
    kasa_office_plug.disable();

    // Air filter control
    air_filter_plug.disable();

    // Subwoofer control
    subwoofer_plug.disable();
    tv_presence.disable();

    // Monitor only
    shed_lights.disable();
    garage_lights.disable();
    freezer_plug.disable();
    car_plug.disable();
    dehumidifier_plug.disable();
    bose_plug.disable();
    vegetable_light.disable();
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

