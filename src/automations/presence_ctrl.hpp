
#ifndef _PRESENCE_CTRL_H_
#define _PRESENCE_CTRL_H_

#include "../modules/kasa.hpp"
#include "../modules/presence.hpp"
#include "automation.hpp"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// This automation turns on a device when a different device is detected.
////////////////////////////////////////////////////////////////////////////////
class presence_ctrl : public automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* kasa_plug;
    presence* presence_obj;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time) {
        int tgt = kasa_plug->get_target();
        int res = kasa_plug->get_status();

        if (tgt == kasa::ON  && res != kasa::ON ) return;
        if (tgt == kasa::OFF && res != kasa::OFF) return;
        if (res != kasa::ON  && res != kasa::OFF) return;

        if (res == kasa::OFF && presence_obj->present())
            kasa_plug->set_target(kasa::ON);
        else if (res == kasa::ON && !presence_obj->present())
            kasa_plug->set_target(kasa::OFF);

    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    presence_ctrl(const char* name, kasa* kasa_plug, presence* presence_obj) {
        char name_full[64];
        snprintf(name_full, 64, "PRESENCE_CTRL [ %s ]", name);
        set_name(name_full);
        this->kasa_plug = kasa_plug;
        this->presence_obj = presence_obj;
        report("constructor done", 3);
    }
};

#endif
