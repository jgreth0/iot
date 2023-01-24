
#ifndef _OUTSIDE_LIGHTS_H_
#define _OUTSIDE_LIGHTS_H_

#include "../modules/kasa.hpp"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// This automation implements a set of rules for tuning on and off outside
// lights.
//
// Users of this codebase are expected to replace this automation with something
// that implements their own desired rules. This is just one example.
////////////////////////////////////////////////////////////////////////////////
class outside_lights : public module {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    int front_ct;
    kasa* front_lights;

    int rear_ct;
    kasa* rear_lights;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Check the device states, including when the most recent state changes
    // occurred, and make changes based on the following rules:
    // - Turn all lights off in the morning
    // - Turn front lights on in the evening
    // - Turn off front lights at night, but only if the back lights are also off
    //
    // If lighting is desired all night, user can turn on a rear light.
    //
    // Use set_sync_time() to avoid unnecessary work.
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        if (last) {
            return;
        }
        bool rear_are_on = false, rear_are_not_off = false;
        time_point current_time = now_floor();
        time_point rear_time = current_time;
        int key_id = get_key_id(current_time);

        for (int i = 0; i < rear_ct; i++) {
            rear_lights[i].heart_beat_missed();
            int tgt = rear_lights[i].get_target();
            int res = rear_lights[i].get_status();
            if (res == kasa::ON) rear_are_on = true;
            if (current_time < rear_lights[i].get_last_time_on() + duration(900)) {
                rear_are_not_off = true;
                if (rear_time < rear_lights[i].get_last_time_on() + duration(900))
                    rear_time = rear_lights[i].get_last_time_on() + duration(900);
            }
            if ((tgt == kasa::ON || tgt == kasa::OFF) && res != tgt) continue;
            if (res == kasa::ON) {
                // Turn off all outside lights in the morning.
                if (rear_lights[i].get_last_time_off() < get_key_time(current_time, 0))
                    rear_lights[i].set_target(kasa::OFF);
            }
        }

        if (!rear_are_on && rear_are_not_off) {
            // None of the rear lights are on, but they haven't been off long
            // enough to be sure that it isn't a momentary error or user error.
            // Wait until all of the lights have been off for at least 15
            // minutes before responding.
            set_sync_time(rear_time);
        }

        for (int i = 0; i < front_ct; i++) {
            front_lights[i].heart_beat_missed();
            int tgt = front_lights[i].get_target();
            int res = front_lights[i].get_status();
            if ((tgt == kasa::ON || tgt == kasa::OFF) && res != tgt) continue;
            if (res == kasa::ON && key_id == 0) {
                // Turn off all outside lights in the morning.
                if (front_lights[i].get_last_time_off() < get_key_time(current_time, 0))
                    front_lights[i].set_target(kasa::OFF);
            }
            else if (res == kasa::ON && key_id == 2 && !rear_are_not_off) {
                // Turn off front lights at night.
                if (front_lights[i].get_last_time_off() < get_key_time(current_time, 2))
                    front_lights[i].set_target(kasa::OFF);
            }
            else if (res == kasa::OFF && key_id == 1) {
                // Turn the front lights on in the evening.
                if (front_lights[i].get_last_time_on() < get_key_time(current_time, 1))
                    front_lights[i].set_target(kasa::ON);
            }
            else if (res == kasa::OFF && key_id == 2 && rear_are_not_off) {
                // Turn the front lights on at night if the rear lights are on (unlikely case).
                if (front_lights[i].get_last_time_on() < get_key_time(current_time, 1))
                    front_lights[i].set_target(kasa::ON);
            }
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    outside_lights(kasa* front_lights, int front_ct, kasa* rear_lights, int rear_ct) {
        char name_full[64];
        snprintf(name_full, 64, "OUTSIDE_LIGHTS");
        set_name(name_full);
        this->front_lights = front_lights;
        this->front_ct = front_ct;
        this->rear_lights = rear_lights;
        this->rear_ct = rear_ct;

        add_key_time( 7*60);
        add_key_time(16*60 + 45);
        add_key_time(22*60 + 15);

        report("constructor done", 3);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void enable() {
        report("enable()", 4);
        module::enable();
        for (int i = 0; i < front_ct; i++) front_lights[i].listen(this);
        for (int i = 0; i <  rear_ct; i++)  rear_lights[i].listen(this);
        report("enable() done", 4);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void disable() {
        report("disable()", 4);
        for (int i = 0; i < front_ct; i++) front_lights[i].unlisten(this);
        for (int i = 0; i <  rear_ct; i++)  rear_lights[i].unlisten(this);
        module::disable();
        report("disable() done", 4);
    }
};

#endif
