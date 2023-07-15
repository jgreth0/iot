
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

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time, time_point key_time) {

        while (current_time > key_time) {
            std::time_t tt = sc::to_time_t(current_time);
            std::tm ct;
            localtime_r(&tt, &ct);

            tt = sc::to_time_t(key_time);
            std::tm kt;
            localtime_r(&tt, &kt);

            if ((kt.tm_yday == ct.tm_yday) && (kt.tm_year == ct.tm_year))
                break;
            else {
                kt.tm_mday += 1;
                tt = mktime(&kt);
                key_time = std::chrono::floor<duration>(sc::from_time_t(tt));
                set_time(key_time);
            }
        }

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

        set_time(hour, minute);

        report("constructor done", 3);
    }
};

#endif
