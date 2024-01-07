
#ifndef _TIME_CONDITIONAL_AUTOMATION_H_
#define _TIME_CONDITIONAL_AUTOMATION_H_

#include "time_automation.hpp"
#include <cstring>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class time_conditional_automation : public time_automation {
public:
    static inline const int BEFORE = 0;
    static inline const int AFTER = 1;

private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    int range;
    automation* automation_obj;
    int hour, minute;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time, time_point key_time) {

        if (range == BEFORE) {
            if (current_time < key_time) {
                report("Before, pass", 5);
                automation_obj->sync(current_time);
            } else {
                report("Before, fail", 5);
            }
        } else if (range == AFTER) {
            if (current_time >= key_time) {
                report("After, pass", 5);
                automation_obj->sync(current_time);
            } else {
                report("After, fail", 5);
            }
        }

        set_time(hour, minute);
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    time_conditional_automation(const char* name, automation* automation_obj,
            int range = BEFORE, int hour = 0, int minute = 0, sun_time_fetcher* stf = NULL) {
        char name_full[64];
        snprintf(name_full, 64, "CONDITIONAL [ %s ]", name);
        set_name(name_full);
        this->automation_obj = automation_obj;
        this->range = range;
        this->hour = hour;
        this->minute = minute;

        set_snap(stf);
        set_time(hour, minute);

        report("constructor done", 3);
    }
};

#endif
