
#ifndef _SUN_TIME_FETCHER_H_
#define _SUN_TIME_FETCHER_H_

#include "module.hpp"
#include "json_fetcher.hpp"
#include <cstring>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class sun_time_fetcher : public module {
public:
    time_point key_time;

private:
    std::vector<bool> sunsets;
    bool sunset;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        char log_str[256];

        time_point current_time = now_floor();

        std::time_t tt = sc::to_time_t(current_time);
        std::tm ct;
        localtime_r(&tt, &ct);

        tt = sc::to_time_t(key_time);
        std::tm kt;
        localtime_r(&tt, &kt);

        if ((kt.tm_yday == ct.tm_yday) && (kt.tm_year == ct.tm_year)) return;

        char url[256];
        if (sunset)
            snprintf(url, 256, "%s%04d-%02d-%02d%s%04d-%02d-%02d",
                "https://api.open-meteo.com/v1/forecast?latitude=42.3584&longitude=-71.0598&daily=sunset&timeformat=unixtime&timezone=America/New_York&start_date=",
                ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday, 
                "&end_date=",
                ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday);
        else
            snprintf(url, 256, "%s%04d-%02d-%02d%s%04d-%02d-%02d",
                "https://api.open-meteo.com/v1/forecast?latitude=42.3584&longitude=-71.0598&daily=sunrise&timeformat=unixtime&timezone=America/New_York&start_date=",
                ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday, 
                "&end_date=",
                ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday);

        json_fetcher jf = json_fetcher(url);

        char* str_start;
        char* str_end;

        while (true) {
            if (sunset) {
                str_start = strstr(jf.raw_str, "sunset\":[");
                if (!str_start) break;
                str_start += 9;
            } else {
                str_start = strstr(jf.raw_str, "sunrise\":[");
                if (!str_start) break;
                str_start += 10;
            }
            str_end = strstr(str_start, "]");
            if (!str_end) break;
            tt = (time_t)strtol(str_start, &str_end, 10);
            localtime_r(&tt, &kt);
            if ((kt.tm_yday != ct.tm_yday) || (kt.tm_year != ct.tm_year)) break;

            key_time = std::chrono::floor<duration>(sc::from_time_t(tt));

            if (sunset)
                snprintf(log_str, 256, "Sunset time: %d:%d;", kt.tm_hour, kt.tm_min);
            else
                snprintf(log_str, 256, "Sunrise time: %d:%d;", kt.tm_hour, kt.tm_min);
            report(log_str, 2, true);

            return;
        }
        fflush(stdout);
        snprintf(log_str, 256, "Failed to parse sun time: %s;", jf.raw_str);
        report(log_str, 2);
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    sun_time_fetcher(bool sunset = true) : module(true, 60*60) {
        char name_full[64];
        if (sunset)
            snprintf(name_full, 64, "SUNSET_TIME_FETCHER");
        else
            snprintf(name_full, 64, "SUNRISE_TIME_FETCHER");
        set_name(name_full);
        this->sunset = sunset;
        this->key_time = now_floor() - duration(60*60*48);
        report("constructor done", 3);
    }
};

#endif
