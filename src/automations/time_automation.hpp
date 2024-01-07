
#ifndef _TIME_AUTOMATION_H_
#define _TIME_AUTOMATION_H_

#include "automation.hpp"
#include "../modules/sun_time_fetcher.hpp"

////////////////////////////////////////////////////////////////////////////////
// Sends a command at a specified time
////////////////////////////////////////////////////////////////////////////////
class time_automation : public automation {
private:
    time_point key_time;
    std::mutex mtx;
    sun_time_fetcher* snap_source = NULL;

public:
    virtual void sync(time_point current_time, time_point key_time) {}

    void sync(time_point current_time) {
        std::unique_lock<std::mutex> lck(mtx);
        time_point kt = key_time;
        lck.unlock();
        sync(current_time, kt);
    }

    void set_time(int hour = 0, int minute = 0) {
        std::unique_lock<std::mutex> lck(mtx);

        time_point current_time = now_floor();
        std::time_t tt = sc::to_time_t(current_time);
        std::tm lt;
        localtime_r(&tt, &lt);

        lt.tm_hour = hour;
        lt.tm_min = minute;

        tt = mktime(&lt);
        key_time = std::chrono::floor<duration>(sc::from_time_t(tt));

        if (snap_source &&
                snap_source->key_time < key_time + duration(8*60*60) &&
                key_time < snap_source->key_time + duration(8*60*60)) {
            // We are within 8 hours. Snap time.
            key_time = snap_source->key_time;
        }
    }

    void set_snap(sun_time_fetcher* snap_source) {
        std::unique_lock<std::mutex> lck(mtx);
        this->snap_source = snap_source;
    }
};

#endif
