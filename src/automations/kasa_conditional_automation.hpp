
#ifndef _KASA_CONDITIONAL_AUTOMATION_H_
#define _KASA_CONDITIONAL_AUTOMATION_H_

#include "../modules/kasa.hpp"
#include "automation.hpp"
#include <cstring>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class kasa_conditional_automation : public automation {
public:
    static inline const int OR = 0;
    static inline const int AND = 1;

private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    std::vector<kasa*> kasa_plugs;
    int target;
    int combination;
    int delay;
    automation* automation_obj;
    std::mutex mtx;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time) {
        std::unique_lock<std::mutex> lck(mtx);

        bool do_sync = false;

        if (combination == OR) {
            do_sync = false;
            for (int i = 0; i < kasa_plugs.size(); i++) {
                kasa_plugs[i]->heart_beat_missed();
                if ((target == kasa::ON) && (kasa_plugs[i]->get_last_time_on() + duration(60 * delay) >= current_time)) {
                    // The plug was ON at some point within the last few minutes.
                    do_sync = true;
                } else if ((target == kasa::OFF) && (kasa_plugs[i]->get_last_time_off() + duration(60 * delay) >= current_time)) {
                    // The plug was OFF at some point within the last few minutes.
                    do_sync = true;
                }
            }
        } else if (combination == AND) {
            do_sync = true;
            for (int i = 0; i < kasa_plugs.size(); i++) {
                kasa_plugs[i]->heart_beat_missed();
                if ((target == kasa::ON) && (kasa_plugs[i]->get_last_time_off() + duration(60 * delay) >= current_time)) {
                    // The plug was off at any point within the last few minutes.
                    do_sync = false;
                } else if ((target == kasa::OFF) && (kasa_plugs[i]->get_last_time_on() + duration(60 * delay) >= current_time)) {
                    // The plug was ON at some point within the last few minutes.
                    do_sync = false;
                }
            }

        }

        if (do_sync) {
            automation_obj->sync(current_time);
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    kasa_conditional_automation(const char* name, automation* automation_obj,
            int target = kasa::OFF, int combination = AND, int delay = 15) {
        char name_full[64];
        snprintf(name_full, 64, "CONDITIONAL [ %s ]", name);
        set_name(name_full);
        this->automation_obj = automation_obj;
        this->target = target;
        this->combination = combination;
        this->delay = delay;
        report("constructor done", 3);
    }

    void add_plug(kasa* k) {
        std::unique_lock<std::mutex> lck(mtx);
        kasa_plugs.push_back(k);
    }
};

#endif
