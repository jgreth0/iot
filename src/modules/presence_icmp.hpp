
#ifndef _PRESENCE_ICMP_H_
#define _PRESENCE_ICMP_H_

#include "presence.hpp"
#include "icmp_helper.hpp"

////////////////////////////////////////////////////////////////////////////////
// this module detects the presence of a device on the network using Internet
// Control Message Protocol (ICMP) Echo requests and Responses (ping).
////////////////////////////////////////////////////////////////////////////////
class presence_icmp : public presence {
private:
    icmp_helper ping;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false) {
        char report_str[256];

        if (last) {
            update_last();
            return;
        }

        // Wait a random amount, up to 500ms, to avoid bursts.
        usleep(1000 * (rand() % 500));

        if (ping.ping()) {
            // The device was seen.
            update_present(now_floor());
        } else {
            update_not_present();
        }
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    presence_icmp(char* name, char* addr, int time_limit = 300) : ping{addr} {
        char name_full[64];
        snprintf(name_full, 64, "PRESENCE ICMP [ %s @ %s ]", name, addr);
        set_name(name_full);
        this->time_limit = duration(time_limit);

        last_time_present = scan_report("DEVICE_PRESENT");
        last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

        report("constructor done", 3);
    }
    presence_icmp(const char* name, const char* addr, int time_limit = 300) : ping{addr} {
        ping = icmp_helper(addr);
        char name_full[64];
        snprintf(name_full, 64, "PRESENCE ICMP [ %s @ %s ]", name, addr);
        set_name(name_full);
        this->time_limit = duration(time_limit);

        last_time_present = scan_report("DEVICE_PRESENT");
        last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

        report("constructor done", 3);
    }
};

#endif
