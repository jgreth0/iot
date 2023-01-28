
#ifndef _SWITCH_PLUG_H_
#define _SWITCH_PLUG_H_

#include "automation.hpp"
#include "../modules/kasa.hpp"

#include <execinfo.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// This automation implements a set of rules for controlling a smart plug with a
// smart switch. This is useful in case some lamps are connected to an outlet
// which is not controlled by the room switch. This automation simulates a
// direct switch/outlet connection. This also provides an opportunity to turn
// the lights on and off automatically without breaking the switch/outlet
// connection. (When using a smart plug and a conventional switch, the switch
// cannot turn the light on when the plug is off. With this setup, there is no
// such limitation.)
//
// Users of this codebase are expected to replace this automation with something
// that implements their own desired rules. This is just one example.
////////////////////////////////////////////////////////////////////////////////
class switch_plug : public automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* kasa_switch;
    kasa* kasa_plug;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Check the device states, including when the most recent state changes
    // occurred, and make changes.
    //
    // - The kasa plug and the kasa switch are linked. When one changes, the
    // other is updated to match.
    // - The bedroom switch and plugs are similarly linked, but with extra rules.
    // - The bedroom switch is only ever changed if both the plugs have already
    // toggled.
    // - bed_plug_high is a dimmer which fades on in the morning and fades off
    // in the evening.
    // - bed_plug_high is turned off at night and is not responsive to switch
    // toggles at night.
    // - both bed lights turn on automatically in the morning (wake up alarm).
    // - bed_plug_low turns on automatically in the evening (security light).
    // - both bed lights turn off automatically after 4 hours.
    //
    // Use set_sync_time() to avoid unnecessary work.
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time) {
        kasa_switch->heart_beat_missed();
        kasa_plug->heart_beat_missed();

        // Query values once and use the returned values from here on. This
        // reduces the risk that something will change in the middle of
        // processing.
        int kasa_switch_tgt = kasa_switch->get_target();
        int kasa_switch_res = kasa_switch->get_status();
        int kasa_plug_tgt   = kasa_plug->get_target();
        int kasa_plug_res   = kasa_plug->get_status();

        if (kasa_switch_tgt == kasa::ON  && kasa_switch_res != kasa::ON ) return;
        if (  kasa_plug_tgt == kasa::ON  &&   kasa_plug_res != kasa::ON ) return;
        if (kasa_switch_tgt == kasa::OFF && kasa_switch_res != kasa::OFF) return;
        if (  kasa_plug_tgt == kasa::OFF &&   kasa_plug_res != kasa::OFF) return;
        if (kasa_switch_res != kasa::ON  && kasa_switch_res != kasa::OFF) return;
        if (  kasa_plug_res != kasa::ON  &&   kasa_plug_res != kasa::OFF) return;

        time_point kasa_switch_time = (kasa_switch_res == kasa::ON ) ? kasa_switch->get_last_time_off() :
                                        kasa_switch->get_last_time_on();
        time_point   kasa_plug_time = (kasa_plug_res == kasa::ON ) ? kasa_plug->get_last_time_off() :
                                        kasa_plug->get_last_time_on();

        if ((kasa_switch_res != kasa_plug_res) &&
                (kasa_switch_time > kasa_plug_time)) {
            // Turn lights on/off per switch
            kasa_plug->set_target(kasa_switch_res);
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    // Once enabled, this object will apply the following rules:
    // - When the media device turns on, turn the air filter off.
    // - When the air filter has been on for 'on_time' time, turn it off.
    // - When the air filter has been off for 'off_time' time, and the media
    //   device is off, turn the air filter on.
    // - If, at 3pm, the TV is off, turn the filter on (this resets the daily
    //   routine).
    ////////////////////////////////////////////////////////////////////////////
    switch_plug(const char* name, kasa* kasa_switch, kasa* kasa_plug) {
        char name_full[64];
        snprintf(name_full, 64, "SWITCH_PLUG [ %s ]", name);
        set_name(name_full);
        this->kasa_switch = kasa_switch;
        this->kasa_plug   = kasa_plug;

        report("constructor done", 3);
    }
};

#endif
