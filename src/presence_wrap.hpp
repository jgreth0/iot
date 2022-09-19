
#ifndef _PRESENCE_WRAP_H_
#define _PRESENCE_WRAP_H_

#include "presence.hpp"

////////////////////////////////////////////////////////////////////////////////
// This module simply ORs the present() results from an array of other presence
// modules. The only source of complexity is in correctly computing
// get_last_time_not_present().
////////////////////////////////////////////////////////////////////////////////
class presence_wrap : public presence {
private:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    time_point last_time_not_present;
    std::mutex mtx;
    bool presence_reported = false, last_reported = false;

    presence** modules;
    int module_count = 0;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        if (last) {
            report("DEVICE_PRESENCE_UNKNOWN", 2, true);
            return;
        }
        present();
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    bool present() {
        bool result = false;
        time_point last_time_not_present = time_point(duration(0));
        for (int i = 0; i < module_count; i++) {
            if (modules[i]->present()) {
                result = true;
                time_point ltnp = modules[i]->get_last_time_not_present();
                if (last_time_not_present < ltnp && modules[i]->present())
                    last_time_not_present = ltnp;
            }
        }

        std::unique_lock<std::mutex> lck(mtx);
        if (!presence_reported || (last_reported != result)) {
            this->last_time_not_present = last_time_not_present;
            if (result) report("DEVICE_PRESENT", 2, true);
            else report("DEVICE_NOT_PRESENT", 2, true);
            presence_reported = true;
            last_reported = result;
            notify_listeners();
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last detected
    // If the device was detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_present() {
        time_point result = time_point(duration(0));
        for (int i = 0; i < module_count; i++) {
            if (modules[i]->present()) return modules[i]->now_floor();
            if (modules[i]->get_last_time_present() > result)
                result = modules[i]->get_last_time_present();
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last undetected for at least time_limit seconds?
    // If the device was not detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_not_present() {
        present();
        std::unique_lock<std::mutex> lck(mtx);
        return last_time_not_present;
    }

    presence_wrap(char* name, presence** modules, int count) :
            presence { false } {
        char name_full[64];
        snprintf(name_full, 64, "PRESENCE WRAP [ %s ]", name);
        set_name(name_full);
        module_count = count;
        this->modules = (presence**)calloc(module_count, sizeof(presence*));
        for (int i = 0; i < module_count; i++)
            this->modules[i] = modules[i];
    }

    ~presence_wrap() {
        free(modules);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        for (int i = 0; i < module_count; i++)
            modules[i]->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        for (int i = 0; i < module_count; i++)
            modules[i]->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
