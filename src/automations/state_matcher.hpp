
#ifndef _STATE_MATCHER_H_
#define _STATE_MATCHER_H_

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
// This automation causes several smart plugs/switches to stay the same state.
// When one changes, the rest are updated to match.
////////////////////////////////////////////////////////////////////////////////
class state_matcher : public automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    std::vector<kasa*> kasa_plugs;
    std::mutex mtx;
    // Do not consider changes before this time.
    time_point block_time;
    duration block_length;

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
        std::unique_lock<std::mutex> lck(mtx);

        int target = kasa::ERROR;

        for (int i = 0; i < kasa_plugs.size(); i++) {
            kasa_plugs[i]->heart_beat_missed();
            if ((kasa_plugs[i]->get_last_time_off() > block_time) &&
                    (kasa_plugs[i]->get_last_time_on() > block_time)) {
                int status = kasa_plugs[i]->get_status();
                if ((status == kasa::ON) || (status == kasa::OFF)) {
                    target = status;
                }
            }
        }

        if ((target == kasa::ON) || (target == kasa::OFF)) {
            block_time += block_length;
            if (block_time < current_time)
                block_time = current_time;
            for (int i = 0; i < kasa_plugs.size(); i++) {
                kasa_plugs[i]->set_target(target);
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
    state_matcher(const char* name, duration block_length = duration(30)) {
        char name_full[64];
        snprintf(name_full, 64, "STATE_MATCHER [ %s ]", name);
        set_name(name_full);

        this->block_length = block_length;
        block_time = now_floor() + block_length;

        report("constructor done", 3);
    }

    void add_plug(kasa* k) {
        std::unique_lock<std::mutex> lck(mtx);
        kasa_plugs.push_back(k);
    }
};

#endif
