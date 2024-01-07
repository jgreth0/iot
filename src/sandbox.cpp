
#include "modules/json_fetcher.hpp"
#include "modules/sun_time_fetcher.hpp"

#include <execinfo.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

    char url[256];
    // snprintf(url, 256, "%s", "httpss://api.weather.gov/stations/kbed/observations/latest");
    snprintf(url, 256, "%s", "https://api.open-meteo.com/v1/forecast?latitude=42.3584&longitude=-71.0598&daily=sunrise,sunset&timeformat=unixtime&timezone=America/New_York&start_date=2023-10-08&end_date=2023-10-08");
    printf("%s\n\n", url);
    json_fetcher jf = json_fetcher(url);

    return 0;
}
