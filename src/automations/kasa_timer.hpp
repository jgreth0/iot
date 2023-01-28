
#ifndef _KASA_TIMER_H_
#define _KASA_TIMER_H_

#include "automation.hpp"
#include "../modules/kasa.hpp"

////////////////////////////////////////////////////////////////////////////////
// Turn off the device after a set period
////////////////////////////////////////////////////////////////////////////////
class kasa_timer : public automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* plug;
    int target;
    duration dur;

public:
    void sync(time_point current_time) {
        plug->heart_beat_missed();

        // Query values once and use the returned values from here on. This
        // reduces the risk that something will change in the middle of
        // processing.
        int tgt = plug->get_target();
        int res = plug->get_status();

        if (tgt == kasa::ON  && res != kasa::ON ) return;
        if (tgt == kasa::OFF && res != kasa::OFF) return;
        if (res != kasa::ON  && res != kasa::OFF) return;

        if (target == kasa::ON  && plug->get_last_time_on()  + dur <= current_time)
            plug->set_target(target);

        if (target == kasa::OFF && plug->get_last_time_off() + dur <= current_time)
            plug->set_target(target);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    kasa_timer(kasa* plug, const char* name = "NULL", int target = kasa::ON, int hour = 0, int minute = 0) {
        char name_full[64];
        snprintf(name_full, 64, "ALARM [ %s ]", name);
        set_name(name_full);

        this->plug   = plug;
        this->target = target;
        this->dur    = duration(60*60*hour + 60*minute);

        report("constructor done", 3);
    }
};

#endif
