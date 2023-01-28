
#ifndef _TIME_CONDITIONAL_AUTOMATION_H_
#define _TIME_CONDITIONAL_AUTOMATION_H_

#include "automation.hpp"
#include <cstring>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class time_conditional_automation : public automation {
public:
    static inline const int BEFORE = 0;
    static inline const int AFTER = 1;

private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    int hour;
    int minute;
    int range;
    automation* automation_obj;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time) {
        std::time_t tt = sc::to_time_t(current_time);
        std::tm lt;
        localtime_r(&tt, &lt);

        if (range == BEFORE) {
            if (lt.tm_hour < hour) automation_obj->sync(current_time);
            else if (lt.tm_hour == hour && lt.tm_min < minute) automation_obj->sync(current_time);
        } else if (range == AFTER) {
            if (lt.tm_hour > hour) automation_obj->sync(current_time);
            else if (lt.tm_hour == hour && lt.tm_min >= minute) automation_obj->sync(current_time);
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    time_conditional_automation(const char* name, automation* automation_obj,
            int range = BEFORE, int hour = 0, int minute = 0) {
        char name_full[64];
        snprintf(name_full, 64, "CONDITIONAL [ %s ]", name);
        set_name(name_full);
        this->automation_obj = automation_obj;
        this->range = range;
        this->hour = hour;
        this->minute = minute;
        report("constructor done", 3);
    }
};

#endif
