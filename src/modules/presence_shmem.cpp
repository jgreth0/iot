
#include "presence_shmem.hpp"
#include <string>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_shmem::sync(bool last) {
    if (last) {
        update_last();
        return;
    }
    time_point last_time_present;
    sm->get_result(&last_time_present);

    update_present(last_time_present);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
presence_shmem::presence_shmem(char* name, char* addr, signal_handler* sh, int time_limit) {
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE shmem [ %s @ %s ]", name, addr);
    set_name(name_full);
    this->time_limit = duration(time_limit);

    last_time_present = scan_report("DEVICE_PRESENT");
    last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

    std::string device = "/var/iot/presence/" + std::string(addr) + ".txt";
    sm = new shmem(device.c_str(), sizeof(time_point), sh);

    report("constructor done", 3);
}

presence_shmem::presence_shmem(const char* name, const char* addr, signal_handler* sh, int time_limit) {
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE shmem [ %s @ %s ]", name, addr);
    set_name(name_full);
    this->time_limit = duration(time_limit);

    last_time_present = scan_report("DEVICE_PRESENT");
    last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

    std::string device = "/var/iot/presence/" + std::string(addr) + ".txt";
    sm = new shmem(device.c_str(), sizeof(time_point), sh);

    report("constructor done", 3);
}

presence_shmem::~presence_shmem() {
    sm->~shmem();
}
