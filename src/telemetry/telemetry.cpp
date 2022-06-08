
#include "telemetry.hpp"
#include <cstring>
#include <stdio.h>
#include <csignal>
#include <mutex>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void location::loc2str(char* loc_str, int loc) {
    if (loc == Home) strcpy(loc_str, "home");
    else if (loc == NotHome) strcpy(loc_str, "not home");
    else strcpy(loc_str, "lost");
}

void telemetry::get_name(char* name) {
    strcpy(name, this->name);
}

// Open the file as shared memory
telemetry::telemetry(const char* device, bool notify) :
        module { false } { // Non-automatic updates.
    std::unique_lock<std::mutex> lck(mtx);
    if (shared_mem != nullptr) return;
    sock = open(device, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    shared_mem = (telemetry_values*)mmap(NULL, sizeof(telemetry_values),
        PROT_READ | PROT_WRITE, MAP_SHARED, sock, 0);
    this->notify = notify;
    if (notify) fcntl(sock, F_SETOWN, getpid());
    strcpy(name, "TELEMETRY [ ");
    strcat(name, device);
    strcat(name, " ]");
}

void telemetry::sync_values(telemetry_values* t, bool report_only) {
    std::unique_lock<std::mutex> lck(mtx);
    // Exclusive lock the file to avoid races between processes.
    struct flock flock = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = sizeof(telemetry_values),
        .l_pid = 0};
    fcntl(sock, F_SETLKW, &flock);

    // Make a copy of the old values so we can detect a change.
    telemetry_values old_t;
    memcpy(&old_t, shared_mem, sizeof(telemetry_values));

    // Don't make any changes if report_only is set.
    if (!report_only) {
        // Merge the latest values from the shared and input structures.
        if (shared_mem->merge(t)) {
            // If the shared_mem has changed, signal any listeners.
            kill(fcntl(sock, F_GETOWN), SIGIO);
        }
    }

    // Unlock the file. All accesses to shared_mem are complete.
    flock.l_type = F_UNLCK;
    fcntl(sock, F_SETLK, flock);

    // Report on any changes.
    if (notify) {
        char report_str[256], loc_str[64];
        if (old_t.dad_iphone_home.get_value() != t->dad_iphone_home.get_value()) {
            location::loc2str(loc_str, t->dad_iphone_home.get_value());
            sprintf(report_str, "dad's iPhone is %s", loc_str);
            report(report_str);
        }
        if (old_t.dad_ipad_home.get_value() != t->dad_ipad_home.get_value()) {
            location::loc2str(loc_str, t->dad_ipad_home.get_value());
            sprintf(report_str, "dad's iPad is %s", loc_str);
            report(report_str);
        }
        if (old_t.mom_iphone_home.get_value() != t->mom_iphone_home.get_value()) {
            location::loc2str(loc_str, t->mom_iphone_home.get_value());
            sprintf(report_str, "mom's iPhone is %s", loc_str);
            report(report_str);
        }
        if (old_t.mom_ipad_home.get_value() != t->mom_ipad_home.get_value()) {
            location::loc2str(loc_str, t->mom_ipad_home.get_value());
            sprintf(report_str, "mom's iPad is %s", loc_str);
            report(report_str);
        }
    }
}

void telemetry::sync(bool last) {
    std::unique_lock<std::mutex> lck(mtx);
    if (last) {
        // Sync without writing any new values.
        // This will trigger a report of all unknown/lost values.
        telemetry_values tv;
        sync_values(&tv, true);

        // ~telemetry_sync()
        if (shared_mem == nullptr) return;
        munmap(shared_mem, sizeof(telemetry_values));
        shared_mem = nullptr;
        close(sock);
    }
    else {
        // Sync without writing any new values.
        // This will trigger a report for anything that has changed.
        telemetry_values tv;
        sync_values(&tv, false);
    }
}
