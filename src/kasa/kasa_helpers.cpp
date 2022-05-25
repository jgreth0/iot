
#include "kasa.hpp"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////
// Start the KASA runtime. Reads the config file and initializes each
// device.
///////////////////////////////////////////////////////////////////////////
void kasa::load_conf(char* log_file, char* conf_file, int verbosity) {
    char addr[64], name[64];
    if (verbosity >= 1) printf("KASA: Loading kasa.conf\n");
    FILE* f = fopen(conf_file, "r");
    while (2 == fscanf(f, "%63s %63s", name, addr)) {
        device_map.emplace(std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(log_file, name, addr, verbosity));
    }
    fclose(f);
    if (verbosity >= 1) printf("KASA: Load complete\n");
}

void kasa::stop_all() {
    device_map.clear();
}

///////////////////////////////////////////////////////////////////////////
// Look up the named device and perform the requested operation. This
// works for devices initialized with load_conf()
///////////////////////////////////////////////////////////////////////////
int kasa::get_status(char* name) {
    return device_map.at(std::string(name)).get_status();
}
void kasa::set_target(char* name, int tgt) {
    device_map.at(std::string(name)).set_target(tgt);
}
void kasa::force_sync(char* name) {
    device_map.at(std::string(name)).force_sync();
}
void kasa::force_sync_all() {
    // Begin the process for all devices
    for (auto it = device_map.begin(); it != device_map.end(); it++) {
        std::unique_lock<std::mutex> lck(it->second.mtx);
        it->second.fs_x = true;
        it->second.mt_cv.notify_all();
    }
    // wait for the process to complete for all devices
    for (auto it = device_map.begin(); it != device_map.end(); it++) {
        std::unique_lock<std::mutex> lck(it->second.mtx);
        while(it->second.fs_x) it->second.fs_cv.wait(lck);
    }
}
