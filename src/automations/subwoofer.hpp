
#ifndef _SUBWOOFER_H_
#define _SUBWOOFER_H_

#include "../modules/kasa.hpp"
#include "../modules/presence.hpp"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// This automation turns on the subwoofer plug when the TV is detected on.
// When idle, the subwoofer draws 8 watts. It also makes a crackling sound when
// the driving sound bar is off. To resolve these issues, the subwoofer is on a
// smart switch which is turned off when the subwoofer is not in use.
//
// Users of this codebase are expected to replace this automation with something
// that implements their own desired rules. This is just one example.
////////////////////////////////////////////////////////////////////////////////
class subwoofer : public module {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* subwoofer_plug;
    presence* tv_presence;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Check the device states, including when the most recent state changes
    // occurred, and make changes based on the following rules:
    // - When the air filter has been on for 'on_time' time, turn it off.
    // - When the air filter has been off for 'off_time' time, and it is night
    // time, turn the air filter on.
    //
    // Use set_sync_time() to avoid unnecessary work.
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        if (last) {
            return;
        }
        int tgt = subwoofer_plug->get_target();
        int res = subwoofer_plug->get_status();
        if ((tgt == kasa::ON || tgt == kasa::OFF) && res != tgt) {
            report("sync not propagated... pausing", 3);
            // A set target has not propagated yet. Don't make any more changes
            // until after that has completed. The kasa module will notify and
            // trigger a re-sync after the plug has been updated.
            set_sync_time(now_floor() + duration(24 * 3600));
            return;
        }
        if (res != kasa::ON && res != kasa::OFF) {
            report("Device is in unknown state... pausing", 3);
            // The kasa module will notify and trigger a re-sync after the plug has
            // been updated.
            set_sync_time(now_floor() + duration(24 * 3600));
            return;
        }
        if (res == kasa::OFF && tv_presence->present())
            subwoofer_plug->set_target(kasa::ON);
        else if (res == kasa::ON && !tv_presence->present())
            subwoofer_plug->set_target(kasa::OFF);

    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    subwoofer(kasa* subwoofer_plug, presence* tv_presence) :
            module { false } {
        char name_full[64];
        snprintf(name_full, 64, "SUBWOOFER");
        set_name(name_full);
        this->subwoofer_plug = subwoofer_plug;
        this->tv_presence = tv_presence;
        report("constructor done", 3);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        subwoofer_plug->listen(this);
        tv_presence->listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        subwoofer_plug->unlisten(this);
        tv_presence->unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
