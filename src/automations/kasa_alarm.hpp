
#ifndef _KASA_ALARM_H_
#define _KASA_ALARM_H_

#include "time_automation.hpp"
#include "../modules/kasa.hpp"

////////////////////////////////////////////////////////////////////////////////
// Sends a command at a specified time
////////////////////////////////////////////////////////////////////////////////
class kasa_alarm : public time_automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* plug;
    int target, timeout;
    int hour, minute;

public:
    void sync(time_point current_time, time_point key_time) {
        plug->heart_beat_missed();

        // Query values once and use the returned values from here on. This
        // reduces the risk that something will change in the middle of
        // processing.
        int tgt = plug->get_target();
        int res = plug->get_status();

        if (tgt == kasa::ON  && res != kasa::ON ) return;
        if (tgt == kasa::OFF && res != kasa::OFF) return;
        if (res != kasa::ON  && res != kasa::OFF) return;

        time_point switch_time = (res == kasa::ON ) ? plug->get_last_time_off() :
                                 (res == kasa::OFF) ? plug->get_last_time_on()  :
                                                      current_time;

        if (current_time >= key_time && key_time >= switch_time && target != res &&
                ((timeout <= 0) || ((current_time - key_time) <= duration(timeout * 60)))) {
            plug->set_target(target);
        }

        set_time(hour, minute);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    kasa_alarm(kasa* plug, const char* name = "NULL", int target = kasa::ON,
            int hour = 0, int minute = 0, int timeout = 0, sun_time_fetcher* stf = NULL) {
        char name_full[64];
        snprintf(name_full, 64, "ALARM [ %s ]", name);
        set_name(name_full);

        this->plug    = plug;
        this->target  = target;
        this->timeout = timeout;
        this->hour = hour;
        this->minute = minute;

        set_snap(stf);
        set_time(hour, minute);

        report("constructor done", 3);
    }

};

#endif
