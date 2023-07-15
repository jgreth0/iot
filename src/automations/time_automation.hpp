
#ifndef _TIME_AUTOMATION_H_
#define _TIME_AUTOMATION_H_

#include "automation.hpp"

////////////////////////////////////////////////////////////////////////////////
// Sends a command at a specified time
////////////////////////////////////////////////////////////////////////////////
class time_automation : public automation {
private:
    time_point key_time;
    std::mutex mtx;

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

        if (key_time >= current_time) {
            lt.tm_mday -= 1;
            tt = mktime(&lt);
            key_time = std::chrono::floor<duration>(sc::from_time_t(tt));
        }
    }

    void set_time(time_point key_time) {
        std::unique_lock<std::mutex> lck(mtx);
        this->key_time = key_time;
    }

    void set_time(time_point current_time, time_point key_time) {
        std::unique_lock<std::mutex> lck(mtx);
        if (this->key_time > current_time)
            this->key_time = key_time;
    }
};

#endif
