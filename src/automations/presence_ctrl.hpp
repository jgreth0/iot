
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
        bool present = presence_obj->present();

        time_point kasa_plug_time = (res == kasa::ON ) ? kasa_plug->get_last_time_off() :
                                        kasa_plug->get_last_time_on();
        time_point  presence_time = present ? presence_obj->get_last_time_not_present() :
                                        presence_obj->get_last_time_present();

        if (tgt == kasa::ON  && res != kasa::ON ) return;
        if (tgt == kasa::OFF && res != kasa::OFF) return;
        if (res != kasa::ON  && res != kasa::OFF) return;

        int new_tgt = present ? kasa::ON : kasa::OFF;

        if ((res != new_tgt) && (kasa_plug_time < presence_time))
            kasa_plug->set_target(new_tgt);
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
