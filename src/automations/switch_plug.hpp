
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
class switch_plug : public module {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* kasa_bed_switch;
    kasa* kasa_bed_plug_low;
    kasa* kasa_bed_plug_high;
    kasa* kasa_office_switch;
    kasa* kasa_office_plug;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Check the device states, including when the most recent state changes
    // occurred, and make changes.
    //
    // - The office plug and the office switch are linked. When one changes, the
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
    void sync(bool last = false) {
        if (last) {
            return;
        }

        kasa_bed_switch->heart_beat_missed();
        kasa_bed_plug_low->heart_beat_missed();
        kasa_bed_plug_high->heart_beat_missed();
        kasa_office_switch->heart_beat_missed();
        kasa_office_plug->heart_beat_missed();

        time_point current_time = now_floor();

        // Query values once and use the returned values from here on. This
        // reduces the risk that something will change in the middle of
        // processing.
        int bed_switch_tgt    = kasa_bed_switch->get_target();
        int bed_switch_res    = kasa_bed_switch->get_status();
        int bed_plug_low_tgt  = kasa_bed_plug_low->get_target();
        int bed_plug_low_res  = kasa_bed_plug_low->get_status();
        int bed_plug_high_tgt = kasa_bed_plug_high->get_target();
        int bed_plug_high_res = kasa_bed_plug_high->get_status();
        int office_switch_tgt = kasa_office_switch->get_target();
        int office_switch_res = kasa_office_switch->get_status();
        int office_plug_tgt   = kasa_office_plug->get_target();
        int office_plug_res   = kasa_office_plug->get_status();

        if (   bed_switch_tgt == kasa::ON  &&    bed_switch_res != kasa::ON )    bed_switch_res = kasa::ERROR;
        if ( bed_plug_low_tgt == kasa::ON  &&  bed_plug_low_res != kasa::ON )  bed_plug_low_res = kasa::ERROR;
        if (bed_plug_high_tgt == kasa::ON  && bed_plug_high_res != kasa::ON ) bed_plug_high_res = kasa::ERROR;
        if (office_switch_tgt == kasa::ON  && office_switch_res != kasa::ON ) office_switch_res = kasa::ERROR;
        if (  office_plug_tgt == kasa::ON  &&   office_plug_res != kasa::ON )   office_plug_res = kasa::ERROR;
        if (   bed_switch_tgt == kasa::OFF &&    bed_switch_res != kasa::OFF)    bed_switch_res = kasa::ERROR;
        if ( bed_plug_low_tgt == kasa::OFF &&  bed_plug_low_res != kasa::OFF)  bed_plug_low_res = kasa::ERROR;
        if (bed_plug_high_tgt == kasa::OFF && bed_plug_high_res != kasa::OFF) bed_plug_high_res = kasa::ERROR;
        if (office_switch_tgt == kasa::OFF && office_switch_res != kasa::OFF) office_switch_res = kasa::ERROR;
        if (  office_plug_tgt == kasa::OFF &&   office_plug_res != kasa::OFF)   office_plug_res = kasa::ERROR;

        time_point    bed_switch_time = (bed_switch_res == kasa::ON ) ? kasa_bed_switch->get_last_time_off() :
                                        (bed_switch_res == kasa::OFF) ? kasa_bed_switch->get_last_time_on()  :
                                                                        current_time;
        time_point  bed_plug_low_time = (bed_plug_low_res == kasa::ON ) ? kasa_bed_plug_low->get_last_time_off() :
                                        (bed_plug_low_res == kasa::OFF) ? kasa_bed_plug_low->get_last_time_on()  :
                                                                          current_time;
        time_point bed_plug_high_time = (bed_plug_high_res == kasa::ON ) ? kasa_bed_plug_high->get_last_time_off() :
                                        (bed_plug_high_res == kasa::OFF) ? kasa_bed_plug_high->get_last_time_on()  :
                                                                           current_time;
        time_point office_switch_time = (office_switch_res == kasa::ON ) ? kasa_office_switch->get_last_time_off() :
                                        (office_switch_res == kasa::OFF) ? kasa_office_switch->get_last_time_on()  :
                                                                           current_time;
        time_point   office_plug_time = (office_plug_res == kasa::ON ) ? kasa_office_plug->get_last_time_off() :
                                        (office_plug_res == kasa::OFF) ? kasa_office_plug->get_last_time_on()  :
                                                                         current_time;

        bool    bed_switch_error = !(bed_switch_res    == kasa::ON || bed_switch_res    == kasa::OFF);
        bool  bed_plug_low_error = !(bed_plug_low_res  == kasa::ON || bed_plug_low_res  == kasa::OFF);
        bool bed_plug_high_error = !(bed_plug_high_res == kasa::ON || bed_plug_high_res == kasa::OFF);
        bool office_switch_error = !(office_switch_res == kasa::ON || office_switch_res == kasa::OFF);
        bool   office_plug_error = !(office_plug_res   == kasa::ON || office_plug_res   == kasa::OFF);

        // Determine the key time
        int key_id = get_key_id(current_time);

        // Set dimmer states
        if (key_id == 0) {
            kasa_bed_plug_high->set_brightness_target(1, 35,
                get_key_time(current_time, 0),
                get_key_time(current_time, 0) + duration(15*60));
        } else if (key_id == 1) {
            kasa_bed_plug_high->set_brightness_target(100, 1,
                get_key_time(current_time, 2) + duration(24*60*60),
                get_key_time(current_time, 2) + duration(24*60*60 + 15*60));
        } else if (key_id == 2) {
            kasa_bed_plug_high->set_brightness_target(35, 1,
                get_key_time(current_time, 2),
                get_key_time(current_time, 2) + duration(15*60));
        } else {
            kasa_bed_plug_high->set_brightness_target(1, 35,
                get_key_time(current_time, 0) + duration(24*60*60),
                get_key_time(current_time, 0) + duration(24*60*60 + 15*60));
        }

        // keys 1 and 2 are only used for dimmer states.
        if (key_id != 3) key_id = 0;

        // Time control
        // Update time and res values to prevent additional updates below.
        if (get_key_time(current_time, key_id) > bed_switch_time) {
            if (bed_switch_res == kasa::OFF) {
                kasa_bed_switch->set_target(kasa::ON);
                bed_switch_res = kasa::ON;
                bed_switch_time = current_time;
            }
        }
        if (get_key_time(current_time, key_id) > bed_plug_low_time) {
            if (bed_plug_low_res == kasa::OFF) {
                kasa_bed_plug_low->set_target(kasa::ON);
                bed_plug_low_res = kasa::ON;
                bed_plug_low_time = current_time;
            }
        }
        if (get_key_time(current_time, key_id) > bed_plug_high_time) {
            if ((bed_plug_high_res == kasa::ON) && (key_id == 3)) {
                kasa_bed_plug_high->set_target(kasa::OFF);
                bed_plug_high_res = kasa::OFF;
                bed_plug_high_time = current_time;
            }
            if ((bed_plug_high_res == kasa::OFF) && (key_id != 3)) {
                kasa_bed_plug_high->set_target(kasa::ON);
                bed_plug_high_res = kasa::ON;
                bed_plug_high_time = current_time;
            }
        }
        if (current_time >= get_key_time(current_time, key_id) + duration(60*60*4)) {
            // Shut off the lights after 4 hours in case we forget or go on vacation.
            if ((bed_plug_low_res == kasa::ON) && (current_time >= bed_plug_low_time + duration(60*60*4))) {
                kasa_bed_plug_low->set_target(kasa::OFF);
                bed_plug_low_res = kasa::OFF;
                bed_plug_low_time = current_time;
            }

            if ((bed_plug_high_res == kasa::ON) && (current_time >= bed_plug_high_time + duration(60*60*4))) {
                kasa_bed_plug_high->set_target(kasa::OFF);
                bed_plug_high_res = kasa::OFF;
                bed_plug_high_time = current_time;
            }
        }

        // Switch control.
        if (!bed_switch_error && !bed_plug_low_error &&
                (bed_switch_res != bed_plug_low_res) &&
                (bed_switch_time > bed_plug_low_time)) {
            // Turn lights on/off per switch
            kasa_bed_plug_low->set_target(bed_switch_res);
            bed_plug_low_res = bed_switch_res;
            bed_plug_low_time = current_time;
        }
        if (!bed_switch_error && !bed_plug_high_error &&
                (bed_switch_res != bed_plug_high_res) &&
                (bed_switch_time > bed_plug_high_time)) {
            // Turn lights on/off per switch
            // High lights don't turn on in the evening.
            if ((key_id != 3) || (bed_switch_res == kasa::OFF)) {
                kasa_bed_plug_high->set_target(bed_switch_res);
                bed_plug_high_res = kasa::OFF;
                bed_plug_high_time = current_time;
            }
        }
        if (!bed_switch_error && !bed_plug_low_error && !bed_plug_high_error &&
                (bed_switch_res != bed_plug_low_res) &&
                (bed_switch_res != bed_plug_high_res) &&
                (bed_plug_low_res == bed_plug_high_res) &&
                ((bed_switch_time < bed_plug_low_time) ||
                 (bed_switch_time < bed_plug_high_time))) {
            // Plugs have changed more recently than the switch.
            // Update the switch to be consistent.
            kasa_bed_switch->set_target(bed_plug_low_res);
            bed_switch_res = bed_plug_low_res;
            bed_switch_time = current_time;
        }

        // Office switch control
        if (!office_switch_error && !office_plug_error &&
                (office_switch_res != office_plug_res) &&
                (office_switch_time > office_plug_time)) {
            // Turn lights on/off per switch
            kasa_office_plug->set_target(office_switch_res);
        }
        if (!office_switch_error && !office_plug_error &&
                (office_switch_res != office_plug_res) &&
                (office_switch_time < office_plug_time)) {
            // Plugs have changed more recently than the switch.
            // Update the switch to be consistent.
            kasa_office_switch->set_target(office_plug_res);
        }

        if (bed_plug_low_res == kasa::ON) {
            if (bed_plug_low_time > get_key_time(current_time, key_id))
                set_sync_time(bed_plug_low_time + duration(60*60*4));
            else
                set_sync_time(get_key_time(current_time, key_id) + duration(60*60*4));
        }
        if (bed_plug_high_res == kasa::ON) {
            if (bed_plug_high_time > get_key_time(current_time, key_id))
                set_sync_time(bed_plug_high_time + duration(60*60*4));
            else
                set_sync_time(get_key_time(current_time, key_id) + duration(60*60*4));
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
    switch_plug(kasa* kasa_bed_switch, kasa* kasa_bed_plug_low, kasa* kasa_bed_plug_high,
            kasa* kasa_office_switch, kasa* kasa_office_plug) {
        char name_full[64];
        snprintf(name_full, 64, "SWITCH_PLUG");
        set_name(name_full);
        this->kasa_bed_switch    = kasa_bed_switch;
        this->kasa_bed_plug_low  = kasa_bed_plug_low;
        this->kasa_bed_plug_high = kasa_bed_plug_high;
        this->kasa_office_switch = kasa_office_switch;
        this->kasa_office_plug   = kasa_office_plug;

        add_key_time(7*60 + 30);
        add_key_time(7*60 + 45);
        add_key_time(20*60 + 45);
        add_key_time(21*60);

        report("constructor done", 3);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        kasa_bed_switch->listen(this);
        kasa_bed_plug_low->listen(this);
        kasa_bed_plug_high->listen(this);
        kasa_office_switch->listen(this);
        kasa_office_plug->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        kasa_bed_switch->unlisten(this);
        kasa_bed_plug_low->unlisten(this);
        kasa_bed_plug_high->unlisten(this);
        kasa_office_switch->unlisten(this);
        kasa_office_plug->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};
