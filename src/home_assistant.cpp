
#include "shmem.hpp"
#include "home_assistant.hpp"
#include <stdio.h>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    ha::telemetry_t t;
    ha::scan_in(stdin, &t);

    shmem iphone = shmem("/tmp/iot_iphone_location", sizeof(ha::location_t));
    iphone.enable();
    iphone.set_target(&t.iphone_home);
    iphone.disable();

    shmem ipad = shmem("/tmp/iot_ipad_location", sizeof(ha::location_t));
    ipad.enable();
    ipad.set_target(&t.ipad_home);
    ipad.disable();

    return 0;
}
