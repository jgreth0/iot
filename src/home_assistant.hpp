
#ifndef _HOME_ASSISTANT_H_
#define _HOME_ASSISTANT_H_

#include <stdio.h>
#include <cstring>

namespace ha {

    typedef int location_t;

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    static const inline location_t Unknown = 1;
    static const inline location_t Home    = 2;
    static const inline location_t NotHome = 3;

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    static inline void loc2str(char* loc_str, location_t loc, int len) {
        if      (loc ==    Home) strncpy(loc_str,     "home", len);
        else if (loc == NotHome) strncpy(loc_str, "not home", len);
        else                     strncpy(loc_str,     "lost", len);
    }

    typedef struct {
        location_t iphone_home;
        location_t ipad_home;
    } telemetry_t;

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    static inline void scan_in(FILE* f, telemetry_t* data) {
        char iphone[64], ipad[64];
        if (2 != fscanf(f, "%s ; %s", iphone, ipad)) return;

        if (     !strcmp(iphone, "not_home")) data->iphone_home = NotHome;
        else if (!strcmp(iphone, "home"    )) data->iphone_home = Home   ;
        else if (!strcmp(iphone, "unknown" )) data->iphone_home = Unknown;
        if (     !strcmp(ipad  , "not_home")) data->ipad_home   = NotHome;
        else if (!strcmp(ipad  , "home"    )) data->ipad_home   = Home   ;
        else if (!strcmp(ipad  , "unknown" )) data->ipad_home   = Unknown;
    }

};

#endif
