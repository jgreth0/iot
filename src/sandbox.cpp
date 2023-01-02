
#include "modules/signal_handler.hpp"
#include "automations/air_filter.hpp"
#include "automations/switch_plug.hpp"
#include "automations/outside_lights.hpp"

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

    char time_str[64];
    time_t time = module::sc::to_time_t(module::time_point(module::duration(0)));
    strftime(time_str, 64, "%c", std::localtime(&time));
    printf("%s\n", time_str);

    return 0;
}
