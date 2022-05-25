
#include "kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <ctime>

const char* kasa::STATES[] =
    {"UNCHANGED", "ON", "OFF", "ERROR", "UNKNOWN"};

std::mutex kasa::log_mtx;
std::map<std::string,kasa> kasa::device_map;

////////////////////////////////////////////////////////////////////////
// Print to the log file.
////////////////////////////////////////////////////////////////////////
void kasa::log(char* text) {
    if (log_file[0]) {
        std::unique_lock<std::mutex> lck(log_mtx);
        FILE* f = fopen(log_file, "a");
        fprintf(f, "%s\n", text);
        fflush(f);
        fclose(f);
    }
    if (VERBOSITY >= 2) printf("KASA: log <- %s\n", text);
}

///////////////////////////////////////////////////////////////////////
// Synchronize the local thread state with the global state.
///////////////////////////////////////////////////////////////////////
bool kasa::sync_state(bool checked, tp5 tm) {
    char final_str[256], time_str[64];

    time_t time = sc::to_time_t(tm);
    strcpy(time_str,ctime(&time));
    time_str[strlen(time_str)-1] = '\0';

    if (done_x) {
        // Force the result to UNKNOWN. This will trigger a log entry
        // indicating that the device is no longer monitored.
        res = UNKNOWN;
    }
    if (res_x != res) {
        // Print an entry in the log to indicate that the value has
        // changed.
        sprintf(final_str, "%s, %s, %s", time_str, name, STATES[res]);
        log(final_str);
    }
    if (checked && fs) {
        // A check was done since the force-sync bit was set.  Clear the
        // force-sync bit and notify the thread that triggered the
        // forced-sync.
        fs_x = false;
        fs_cv.notify_all();
    }
    if (res == tgt_x) {
        // If the user actuates the switch manually, the user's choice
        // should have highest priority. This app should not continually
        // return the switch to the target position.
        // Now that the switch is set and confirmed as requested, don't
        // make any more changes in the future unless tgt_x[id] is
        // updated by the controlling thread.
        tgt_x = UNCHANGED;
        if (VERBOSITY >= 3) {
            printf("KASA: Target value '%s' is verified for %s. ",
                STATES[res], name);
            printf("Reverting to 'UNCHANGED'\n");
        }
    }
    // Record the result in globally visible state
    res_x = res;
    // Copy the latest global state into the local thread context.
    fs = fs_x;
    tgt = tgt_x;
    done = done_x;
    return done_x;
}

///////////////////////////////////////////////////////////////////////
// Each device has it's own thread. This thread is the only one which
// can communicate with that device. The control thread communicates
// with the device threads through global state synchronizations
// (above).
///////////////////////////////////////////////////////////////////////
void kasa::device_management_thread() {
    std::unique_lock<std::mutex> lck(mtx);
    tp5 go_time = std::chrono::ceil<d5>(sc::now());

    if (VERBOSITY >= 4) printf("Management Thread started: %s\n", name);
    sync_state(false, go_time);

    while (!done) {
        if (VERBOSITY >= 5) printf("Thread sleep: %s\n", name);
        mt_cv.wait_until(lck, go_time);
        if (VERBOSITY >= 5) printf("Thread wake: %s\n", name);
        do {
            if (sync_state(false, go_time)) break;
            if (VERBOSITY >= 5)
                printf("Management Thread unlock: %s\n", name);
            // Device communication is not protected by the mutex.
            // Commands can be sent to all devices concurrently.
            lck.unlock();
            // Set and then check the state.
            res = sync_device();
            // Lock before synchronizing again.
            lck.lock();
            if (VERBOSITY >= 5)
                printf("Management Thread locked: %s\n", name);
            // It is important to synchronize immediately every time the
            // lock is acquired to check if the controller is shutting
            // down or if a forced sync is requested.
            if (sync_state(true, go_time)) break;
        // Do not wait if there is immediate work to be done.
        } while (fs || (tgt == ON  && res == OFF) ||
                       (tgt == OFF && res == ON ));
        // The "wait_until" above can be triggered either by a timeout
        // or by an explicit "notify_all()" from the controlling thread.
        // If the thread was triggered early, don't increment the
        // go_time.
        // If the loop took longer than usual, skip over any iterations
        // that have already passed.
        go_time = std::chrono::ceil<d5>(sc::now());
    }
    if (VERBOSITY >= 4) printf("Management Thread returning: %s\n", name);
}

///////////////////////////////////////////////////////////////////////////
// Get the most recent state for a deivce.
///////////////////////////////////////////////////////////////////////////
int kasa::get_status() {
    std::unique_lock<std::mutex> lck(mtx);
    if (done_x) {
        fprintf(stderr, "KASA: Failed to get status\n");
        return ERROR;
    }
    return res_x;
}

///////////////////////////////////////////////////////////////////////////
// Set the target state for a deivce.
///////////////////////////////////////////////////////////////////////////
void kasa::set_target(int tgt) {
    std::unique_lock<std::mutex> lck(mtx);
    if (done_x) {
        fprintf(stderr, "KASA: Failed to set target\n");
        return;
    }
    tgt_x = tgt;
}

///////////////////////////////////////////////////////////////////////////
// Flush the target states to each device and refresh the observed states.
// This function blocks until the operation is complete. get_status()
// returns a fresh value after force_sync() returns.
///////////////////////////////////////////////////////////////////////////
void kasa::force_sync() {
    std::unique_lock<std::mutex> lck(mtx);
    fs_x = true;
    mt_cv.notify_all();
    while(fs_x) fs_cv.wait(lck);
}

///////////////////////////////////////////////////////////////////////////
// Start the KASA runtime.
///////////////////////////////////////////////////////////////////////////
kasa::kasa(char* log_file, char* name, char* addr, int verbosity) {
    std::unique_lock<std::mutex> lck(mtx);
    VERBOSITY = verbosity;
    if (VERBOSITY >= 1) printf("KASA: Constructing %s\n", name);
    strcpy(this->log_file, log_file);
    strcpy(this->addr, addr);
    strcpy(this->name, name);
    if (VERBOSITY >= 1) printf("KASA: Done constructing %s\n", name);
}

///////////////////////////////////////////////////////////////////////////
// Stop the KASA runtime.
///////////////////////////////////////////////////////////////////////////
kasa::~kasa() {
    std::unique_lock<std::mutex> lck(mtx);
    if (VERBOSITY >= 1) printf("KASA: Stopping execution\n");
    done_x = true;
    mt_cv.notify_all();
    lck.unlock();
    thread.join();
    if (VERBOSITY >= 1) printf("KASA: Stop complete\n");
}
