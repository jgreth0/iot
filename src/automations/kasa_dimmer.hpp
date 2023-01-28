
#ifndef _KASA_DIMMER_H_
#define _KASA_DIMMER_H_

#include "automation.hpp"
#include "../modules/kasa.hpp"

////////////////////////////////////////////////////////////////////////////////
// Spams a dimmer command
////////////////////////////////////////////////////////////////////////////////
class kasa_dimmer : public automation {
private:
    ////////////////////////////////////////////////////////////////////////////
    // 'kasa' modules provide status and control interfaces for interacting with
    // the real world. These are passed in through the constructor.
    ////////////////////////////////////////////////////////////////////////////
    kasa* plug;
    int start_brightness, end_brightness;
    int start_time_hour, start_time_minute, duration_seconds;

public:
    void sync(time_point current_time) {
        plug->heart_beat_missed();

        std::time_t tt = sc::to_time_t(current_time);
        std::tm lt;
        localtime_r(&tt, &lt);

        lt.tm_hour = start_time_hour;
        lt.tm_min  = start_time_minute;

        tt = mktime(&lt);
        time_point start_time = std::chrono::floor<duration>(sc::from_time_t(tt));

        plug->set_brightness_target(start_brightness, end_brightness, start_time, start_time + duration(duration_seconds));
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    kasa_dimmer(kasa* plug, const char* name = "NULL", int start_brightness = 100, int end_brightness = 100,
            int start_time_hour = 0, int start_time_minute = 0, int duration_seconds = 0) {
        char name_full[64];
        snprintf(name_full, 64, "DIMMER [ %s ]", name);
        set_name(name_full);

        this->plug              = plug;
        this->start_brightness  = start_brightness;
        this->end_brightness    = end_brightness;
        this->start_time_hour   = start_time_hour;
        this->start_time_minute = start_time_minute;
        this->duration_seconds  = duration_seconds;

        report("constructor done", 3);
    }
};

#endif
