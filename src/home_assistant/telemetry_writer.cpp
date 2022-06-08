
#include "../telemetry/telemetry.hpp"
#include <stdio.h>
#include <cstring>

int main(int argc, char *argv[]) {
    char dad_iphone[64], dad_ipad[64], mom_iphone[64], mom_ipad[64];
    if (4 != scanf("%s ; %s ; %s ; %s",
        dad_iphone, dad_ipad, mom_iphone, mom_ipad)) return -1;

    telemetry_values tv;
    if (     !strcmp(dad_iphone, "not_home")) tv.dad_iphone_home.set_value(location::NotHome);
    else if (!strcmp(dad_iphone, "home"    )) tv.dad_iphone_home.set_value(location::Home);
    else if (!strcmp(dad_iphone, "unknown" )) tv.dad_iphone_home.set_value(location::Unknown);
    if (     !strcmp(dad_ipad  , "not_home"))   tv.dad_ipad_home.set_value(location::NotHome);
    else if (!strcmp(dad_ipad  , "home"    ))   tv.dad_ipad_home.set_value(location::Home);
    else if (!strcmp(dad_ipad  , "unknown" ))   tv.dad_ipad_home.set_value(location::Unknown);
    if (     !strcmp(mom_iphone, "not_home")) tv.mom_iphone_home.set_value(location::NotHome);
    else if (!strcmp(mom_iphone, "home"    )) tv.mom_iphone_home.set_value(location::Home);
    else if (!strcmp(mom_iphone, "unknown" )) tv.mom_iphone_home.set_value(location::Unknown);
    if (     !strcmp(mom_ipad  , "not_home"))   tv.mom_ipad_home.set_value(location::NotHome);
    else if (!strcmp(mom_ipad  , "home"    ))   tv.mom_ipad_home.set_value(location::Home);
    else if (!strcmp(mom_ipad  , "unknown" ))   tv.mom_ipad_home.set_value(location::Unknown);

    telemetry ha = telemetry("/tmp/iot_telemetry", false);
    ha.sync_values(&tv);
    ha.~telemetry();
    return 0;
}
