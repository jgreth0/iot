
#include "modules/json_fetcher.hpp"

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
    snprintf(url, 256, "%s", "https://api.weather.gov/stations/kbed/observations/latest");
    printf(url);
    json_fetcher jf = json_fetcher(url);

    return 0;
}
