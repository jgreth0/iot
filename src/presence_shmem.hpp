
#ifndef _PRESENCE_SHMEM_H_
#define _PRESENCE_SHMEM_H_

#include "presence.hpp"
#include "shmem.hpp"
#include "home_assistant.hpp"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class presence_shmem : public presence {
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    std::mutex mtx;
    shmem* sh;
    bool presence_reported = false, last_reported = false;
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
        report("present() called", 5);
        ha::location_t loc;
        time_point last_seen = sh->get_result(&loc);
        report("present() complete", 5);
        return (loc == ha::Home);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    presence_shmem(char* name, shmem* sh) :
            presence { false } {
        char name_full[64];
        snprintf(name_full, 64, "PRESENCE SHMEM [ %s ]", name);
        set_name(name_full);
        this->sh = sh;
        report("constructor done", 3);
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last detected
    // If the device was detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_present() {
        report("get_last_time_present() called", 5);
        ha::location_t loc;
        time_point last_seen = sh->get_result(&loc);
        report("get_last_time_present() complete", 5);
        if (loc == ha::Home)
            return now_floor();
        return last_seen;
    }

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last undetected for at least time_limit seconds?
    // If the device was not detected within the time limit, returns now_floor()
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_not_present() {
        report("get_last_time_not_present() called", 5);
        ha::location_t loc;
        time_point last_seen = sh->get_result(&loc);
        report("get_last_time_not_present() complete", 5);
        if (loc != ha::Home)
            return now_floor();
        return last_seen;
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        sh->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        sh->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
