
#include "module.hpp"
#include <cstring>
#include <ctime>

void module::management_thread(module* m) {
    std::unique_lock<std::mutex> lck(m->mtx);
    while (true) {
        m->sync_start_count++;
        bool last = m->done;
        lck.unlock();
        m->sync(last);
        lck.lock();
        m->sync_finish_count++;
        m->cv_wt.notify_all();
        if (last) break;
        if (!m->skip_wait) {
            if (m->automatic) {
                uint64_t now = m->now().time_since_epoch().count();
                // Round up.
                now += m->update_frequency;
                now -= 1;
                now /= m->update_frequency;
                now *= m->update_frequency;
                m->cv_mt.wait_until(lck, time_point(duration(now)));
            }
            else m->cv_mt.wait(lck);
        }
        m->skip_wait = false;
    }
}
std::mutex module::log_mtx;
int module::VERBOSITY = 1;
char module::log_file[256];
// The derived class implements the sync capability and any required
// setters and getters. The derived class is expected to protect its own
// state against race conditions.
void module::sync(bool last) {
    report("Sync");
}
void module::get_name(char* name) {
    name[0] = '\0';
}
// Report an event.
void module::report(char* text, int verbosity, bool log) {
    char time_str[64], name[64];
    time_t time = sc::to_time_t(now());
    strcpy(time_str,ctime(&time));
    get_name(name);

    if (log) {
        if (log_file[0]) {
            std::unique_lock<std::mutex> lck(log_mtx);
            FILE* f = fopen(log_file, "a");
            fprintf(f, "%s ; %s ; %s\n", time_str, name, text);
            fflush(f);
            fclose(f);
        }
        if (this->VERBOSITY >= verbosity) {
            printf("%s ; %s ; log <- %s\n", time_str, name, text);
            fflush(stdout);
        }
    }
    else if (this->VERBOSITY >= verbosity) {
        printf("%s ; %s ; %s\n", time_str, name, text);
        fflush(stdout);
    }
}

void module::report(const char* text, int verbosity, bool log) {
    char str[256];
    strcpy(str, text);
    report(str, verbosity, log);
}

// sync_wait() triggers a call to sync() and blocks until that call
// completes. If there is a sync() in progress, it waits for that sync()
// to complete and then launches another sync so that a full cycle is
// completed before returning.
// Any number of users of this module can call sync_wait() concurrently.
void module::sync_wait() {
    std::unique_lock<std::mutex> lck(mtx);
    // Wait for the next iteration to finish
    uint64_t target = sync_start_count + 1;
    skip_wait = true;
    do {
        cv_mt.notify_all();
        // Timeout after 1 second and verify that sync() hasn't
        // completed yet.
        cv_wt.wait_for(lck, duration(1));
    } while (sync_finish_count < target);
}
// sync_now() triggers a call to sync() and returns immediately. If
// there is a sync() in progress, another sync() call is scheduled to
// begin immediately upon return of the current call.
void module::sync_now() {
    std::unique_lock<std::mutex> lck(mtx);
    skip_wait = true;
    cv_mt.notify_all();
}
module::module(bool automatic, int update_frequency) {
    strncpy(this->log_file, log_file, 256);
    this->automatic = automatic;
    this->update_frequency = update_frequency;
    thread = std::thread(&management_thread, this);
}
module::~module() {
    std::unique_lock<std::mutex> lck(mtx);
    done = true;
    skip_wait = true;
    cv_mt.notify_all();
    lck.unlock();
    thread.join();
}
void module::set_verbosity(int VERBOSITY) {
    std::unique_lock<std::mutex> lck(log_mtx);
    module::VERBOSITY = VERBOSITY;
}
void module::set_log_file(char* log_file) {
    std::unique_lock<std::mutex> lck(log_mtx);
    strcpy(module::log_file, log_file);
}
module::time_point module::now() {
    return std::chrono::ceil<duration>(sc::now());
}
