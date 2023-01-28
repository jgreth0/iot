
#ifndef _PRESENCE_H_
#define _PRESENCE_H_

#include "module.hpp"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class presence : public module {
protected:
    ////////////////////////////////////////////////////////////////////////////
    // Status information - updated on every sync()
    // Access must be protected by mutex.
    ////////////////////////////////////////////////////////////////////////////
    time_point last_time_present;
    time_point last_time_not_present;
    duration time_limit;
    std::mutex mtx;
    bool presence_reported = false, last_reported = false;

    virtual void sync(bool last = false) = 0;

    void update_not_present() {
        report("update_not_present() called", 5);
        std::unique_lock<std::mutex> lck(mtx);
        time_point last_time_present = this->last_time_present;
        if ((now_floor() - last_time_present) >= time_limit)
            last_time_not_present = now_floor();
        lck.unlock();

        if ((now_floor() - last_time_present) < time_limit) {
            if (!presence_reported || !last_reported) {
                report("DEVICE_PRESENT", 2, true);
                notify_listeners();
            }
            presence_reported = true;
            last_reported = true;
        } else {
            if (!presence_reported || last_reported) {
                report("DEVICE_NOT_PRESENT", 2, true);
                notify_listeners();
            }
            presence_reported = true;
            last_reported = false;
        }
    }

    void update_present(time_point last_time_present) {
        report("update_present() called", 5);
        std::unique_lock<std::mutex> lck(mtx);
        if ((now_floor() - this->last_time_present) >= time_limit)
            last_time_not_present = now_floor();
        this->last_time_present = last_time_present;
        lck.unlock();

        if ((now_floor() - last_time_present) < time_limit) {
            if (!presence_reported) {
                report("DEVICE_PRESENCE_UNKNOWN", 2, true);
                report("DEVICE_PRESENT", 2, true);
                notify_listeners();
            } else if (!last_reported) {
                report("DEVICE_NOT_PRESENT", 2, true);
                report("DEVICE_PRESENT", 2, true);
                notify_listeners();
            }
            presence_reported = true;
            last_reported = true;
        } else {
            if (!presence_reported) {
                report("DEVICE_PRESENCE_UNKNOWN", 2, true);
                report("DEVICE_NOT_PRESENT", 2, true);
                notify_listeners();
            } else if (last_reported) {
                report("DEVICE_PRESENT", 2, true);
                report("DEVICE_NOT_PRESENT", 2, true);
                notify_listeners();
            }
            presence_reported = true;
            last_reported = false;
        }

    }

    void update_last() {
        if (presence_reported) {
            if (last_reported)
                report("DEVICE_PRESENT", 2, true);
            else
                report("DEVICE_NOT_PRESENT", 2, true);
            report("DEVICE_PRESENCE_UNKNOWN", 2, true);
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    bool present() {
        report("present() called", 5);
        std::unique_lock<std::mutex> lck(mtx);
        bool p = (now_floor() - last_time_present) < time_limit;
        lck.unlock();
        report("present() complete", 5);
        return p;
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last detected
    // If the device was detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_present() {
        report("get_last_time_present() called", 5);
        std::unique_lock<std::mutex> lck(mtx);
        bool p = (now_floor() - last_time_present) < time_limit;
        time_point t = last_time_present;
        if (p) t = now_floor();
        lck.unlock();
        report("get_last_time_present() complete", 5);
        return t;
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last undetected for at least time_limit seconds?
    // If the device was not detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_not_present() {
        report("get_last_time_not_present() called", 5);
        std::unique_lock<std::mutex> lck(mtx);
        bool p = (now_floor() - last_time_present) < time_limit;
        time_point t = last_time_not_present;
        if (!p) t = now_floor();
        lck.unlock();
        report("get_last_time_not_present() complete", 5);
        return t;
    }

    presence(bool automatic = true, int update_frequency = 1) :
        module { automatic, update_frequency } {}
};

#endif
