
#ifndef _AIR_FILTER_H_
#define _AIR_FILTER_H_

#include "module.hpp"
#include "presence.hpp"
#include "kasa.hpp"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// This module implements a set of rules for tuning on and off an air filter
// throughout the day. The air filter is turned on and off at fixed intervals
// with a few exceptions.
//
// Users of this codebase are expected to replace this module with something
// that implements their own desired rules. This is just one example.
////////////////////////////////////////////////////////////////////////////////
class air_filter : public module {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' and 'presence' modules provide status and control interfaces for
    // interacting with the real world. These are passed in through the
    // constructor.
    ////////////////////////////////////////////////////////////////////////////
    presence* media_presence;
    kasa* air_filter_plug;

    ////////////////////////////////////////////////////////////////////////////
    // The time spent in 'ON' and 'OFF' states is configurable. These are passed
    // in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    duration on_dur, off_dur;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Check the device states, including when the most recent state changes
    // occurred, and make changes based on the following rules:
    // - When the TV turns on, turn the air filter off.
    // - When the air filter has been on for 'on_dur' time, turn it off.
    // - When the air filter has been off for 'off_dur' time, and the TV is off,
    //   turn the air filter on.
    //
    // Use set_sync_time() to avoid unnecessary work.
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        if (last) {
            return;
        }
        int tgt = air_filter_plug->get_target();
        int res = air_filter_plug->get_status();
        if ((tgt == kasa::ON || tgt == kasa::OFF) && res != tgt) {
            report("sync not propagated... pausing", 3);
            // A set target has not propagated yet. Don't make any more changes
            // until after that has completed. The kasa module will notify and
            // trigger a re-sync after the plug has been updated.
            set_sync_time(now_floor() + duration(24 * 3600));
            return;
        }
        if (res != kasa::ON && res != kasa::OFF) {
            report("Device is in unknown state... pausing", 3);
            // The kasa module will notify and trigger a re-sync after the plug has
            // been updated.
            set_sync_time(now_floor() + duration(24 * 3600));
            return;
        }
        if (media_presence->present()) {
            if (res == kasa::OFF) {
                report("The TV is on and the device is off. Waiting...", 3);
                // No need to update until the TV is off again
                set_sync_time(now_floor() + duration(24 * 3600));
                return;
            }
            if (media_presence->get_last_time_not_present() > air_filter_plug->get_last_time_off()) {
                report("turning off the device since the TV was turned on", 3);
                air_filter_plug->set_target(kasa::OFF);
                set_sync_time(now_floor() + duration(24 * 3600));
                return;
            }
            report("the device was turned on after the TV was turned on. Applying the time-based condition", 3);
        }

        if (res == kasa::ON) {
            time_point off_time = air_filter_plug->get_last_time_off() + on_dur;

            if (off_time <= now_floor()) {
                report("Device has been on for enough time. Switching off", 3);
                air_filter_plug->set_target(kasa::OFF);
                set_sync_time(now_floor() + off_dur);
            } else {
                report("Device has not been on for enough time. Waiting...", 3);
                set_sync_time(off_time);
            }
        } else {
            time_point on_time = air_filter_plug->get_last_time_on() + off_dur;

            std::time_t tt = sc::to_time_t(on_time);
            std::tm lt;
            localtime_r(&tt, &lt);

            if (lt.tm_hour >= 7 && lt.tm_hour < 18) {
                // on_time is between 7:00am and 6:00pm. Don't run during work hours.
                lt.tm_hour = 18;
                lt.tm_min = 0;
                lt.tm_sec = 0;
                tt = mktime(&lt);
                on_time = std::chrono::floor<duration>(sc::from_time_t(tt));
            }

            if (on_time <= now_floor()) {
                report("Device has been off for enough time. Switching on", 3);
                air_filter_plug->set_target(kasa::ON);
                set_sync_time(now_floor() + on_dur);
            } else {
                report("Device has not been off for enough time. Waiting...", 3);
                set_sync_time(on_time);
            }
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
    air_filter(presence* media_presence, kasa* air_filter_plug,
            int on_time = 10*360, int off_time = 50*360) {
        char name_full[64];
        snprintf(name_full, 64, "AIR_FILTER");
        set_name(name_full);
        this->media_presence = media_presence;
        this->air_filter_plug = air_filter_plug;
        this->on_dur  = duration( on_time);
        this->off_dur = duration(off_time);
        report("constructor done", 3);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        media_presence->listen(this);
        air_filter_plug->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        media_presence->unlisten(this);
        air_filter_plug->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
