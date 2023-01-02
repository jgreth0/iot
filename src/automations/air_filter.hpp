
#ifndef _AIR_FILTER_H_
#define _AIR_FILTER_H_

#include "../modules/kasa.hpp"
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
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
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
    // - When the air filter has been on for 'on_time' time, turn it off.
    // - When the air filter has been off for 'off_time' time, and it is night
    // time, turn the air filter on.
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

            if (lt.tm_hour >= 5) {
                // on_time is between 5:00am and midnight. Don't run.
                lt.tm_hour = 24;
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
    //
    ////////////////////////////////////////////////////////////////////////////
    air_filter(kasa* air_filter_plug,
            int on_time = 5*360, int off_time = 50*360) {
        char name_full[64];
        snprintf(name_full, 64, "AIR_FILTER");
        set_name(name_full);
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
        air_filter_plug->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        air_filter_plug->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
