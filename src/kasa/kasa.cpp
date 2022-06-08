
#include "kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <ctime>

const char* kasa::STATES[] =
    {"UNCHANGED", "ON", "OFF", "ERROR", "UNKNOWN"};

void kasa::get_name(char* name) {
    strcpy(name, this->name);
}

void kasa::sync(bool last) {
    std::unique_lock<std::mutex> lck(mtx);
    int res = UNKNOWN;
    do {
        int tgt = this->tgt;
        lck.unlock();
        res = sync_device(tgt);
        lck.lock();
    } while ((tgt == ON && res == OFF) || (tgt == OFF && res == ON));

    if (last) res = UNKNOWN;

    if (this->res != res) {
        // The value has changed.
        char report_str[256];
        sprintf(report_str, "state: %s", STATES[res]);
        report(report_str, 2, true);
    }
    if (res != tgt) tgt = UNCHANGED;
    this->res = res;
}

///////////////////////////////////////////////////////////////////////////
// Get the most recent state for a deivce.
///////////////////////////////////////////////////////////////////////////
int kasa::get_status() {
    std::unique_lock<std::mutex> lck(mtx);
    return res;
}

///////////////////////////////////////////////////////////////////////////
// Set the target state for a deivce.
///////////////////////////////////////////////////////////////////////////
void kasa::set_target(int tgt) {
    std::unique_lock<std::mutex> lck(mtx);
    this->tgt = tgt;
}

///////////////////////////////////////////////////////////////////////////
// Start the KASA runtime.
///////////////////////////////////////////////////////////////////////////
kasa::kasa(char* name, char* addr) {
    std::unique_lock<std::mutex> lck(mtx);
    strcpy(this->addr, addr);
    strcpy(this->name, "KASA [");
    strcat(this->name, name);
    strcat(this->name, " @ ");
    strcat(this->name, addr);
    strcat(this->name, "]");
}
