
#include "module.hpp"
#include <ctime>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
std::mutex module::log_mtx;
int module::verbosity_limit = 3;
char module::log_file[256] = "";

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::management_thread(module* m) {
    std::unique_lock<std::mutex> lck(m->mtx);
    while (true) {
        m->report("[MODULE] management_thread loop", 5);
        m->default_update = true;
        if (m->key_times.empty()) {
            // Round up.
            uint64_t uf = m->update_frequency;
            m->next_sync_time = time_point(duration(
                ((m->now_ceil().time_since_epoch().count() + uf - 1 ) / uf) * uf));
        } else {
            int id;
            std::time_t tt = sc::to_time_t(m->now_floor());
            std::tm lt;
            localtime_r(&tt, &lt);
            int now = lt.tm_hour * 60 + lt.tm_min;
            for (id = 0; id < m->key_times.size(); id++)
                if (m->key_times[id] > now) break;
            if (id == m->key_times.size()) {
                id = 0;
                lt.tm_mday++;
            }
            lt.tm_hour = m->key_times[id] / 60;
            lt.tm_min = m->key_times[id] % 60;
            tt = mktime(&lt);
            m->next_sync_time = std::chrono::floor<duration>(sc::from_time_t(tt));
        }
        bool last = m->done;
        m->sync_start_count++;
        m->report("[MODULE] calling sync()", 5);
        lck.unlock();
        m->sync(last);
        lck.lock();
        time_point nc = m->now_ceil();
        m->heart_beat_requested = false;
        m->report("[MODULE] sync() done", 4);
        m->cv_wt.notify_all();
        m->sync_finish_count++;
        if (last) break;
        if (!m->skip_wait) {
            m->report("[MODULE] management_thread waiting", 5);
            if (m->next_sync_time < nc) m->next_sync_time = nc;
            if (m->automatic) {
                do {
                    m->cv_mt.wait_until(lck, m->next_sync_time);
                    m->heart_beat_requested = false;
                    m->cv_wt.notify_all();
                } while (!m->skip_wait && m->now_floor() < m->next_sync_time);
            } else {
                do {
                    m->cv_mt.wait(lck);
                    m->heart_beat_requested = false;
                    m->cv_wt.notify_all();
                } while (!m->skip_wait);
            }
        } else {
            m->report("[MODULE] management_thread skipped waiting", 3);
        }
        m->skip_wait = false;
    }
    m->mt_exit = true;
    m->cv_wt.notify_all();
    m->report("[MODULE] management_thread complete", 3);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::set_sync_time(time_point next_sync_time) {
    char st_str[128], time_str[64];
    time_t time = sc::to_time_t(next_sync_time);
    strftime(time_str, 64, "%c", std::localtime(&time));
    sprintf(st_str, "[MODULE] set_sync_time(%s)", time_str);
    report(st_str, 3);
    if (default_update || this->next_sync_time > next_sync_time)
        this->next_sync_time = next_sync_time;
    default_update = false;
}

void module::add_key_time(int min) {
    key_times.push_back(min);
}

int module::get_key_id(time_point current_time) {
    if (key_times.empty()) {
        report("[MODULE] Error: Key ID was requested but not configured.", 3);
        return -1;
    }
    int id;
    std::time_t tt = sc::to_time_t(current_time);
    std::tm lt;
    localtime_r(&tt, &lt);
    int now = lt.tm_hour * 60 + lt.tm_min;
    for (id = 0; id < key_times.size(); id++)
        if (key_times[id] > now) break;
    if (id == 0) id = key_times.size() - 1;
    else id = id - 1;

    char report_str[256], time_str[64];
    strftime(time_str, 64, "%c", std::localtime(&tt));
    sprintf(report_str, "get_key_id(%s): %d;", time_str, id);
    report(report_str, 4);

    return id;
}

module::time_point module::get_key_time(time_point current_time, int id) {
    if (key_times.empty()) {
        report("[MODULE] Error: Key time was requested but not configured.", 3);
        return time_point(duration(0));
    }
    int now_id;
    std::time_t tt = sc::to_time_t(current_time);
    std::tm lt;
    localtime_r(&tt, &lt);
    int now = lt.tm_hour * 60 + lt.tm_min;
    for (now_id = 0; now_id < key_times.size(); now_id++)
        if (key_times[now_id] > now) break;

    if (now_id == 0) {
        lt.tm_mday -= 1;
        if (id == -1) id = key_times.size() - 1;
    } else {
        if (id == -1) id = now_id - 1;
        else if (now_id <= id) lt.tm_mday -= 1;
    }
    lt.tm_hour = key_times[id] / 60;
    lt.tm_min  = key_times[id] % 60;

    std::time_t tt2 = mktime(&lt);

    char report_str[256], time_str1[64], time_str2[64];
    strftime(time_str1, 64, "%c", std::localtime(&tt));
    strftime(time_str2, 64, "%c", std::localtime(&tt2));
    sprintf(report_str, "get_key_time(%s, %d): %s;", time_str1, id, time_str2);
    report(report_str, 4);

    return std::chrono::floor<duration>(sc::from_time_t(tt2));
}

////////////////////////////////////////////////////////////////////////////////
// The derived class implements the sync capability and any required
// setters and getters. The derived class is expected to protect its own
// state against race conditions.
////////////////////////////////////////////////////////////////////////////////
void module::sync(bool last) {
    report("[MODULE] Sync", 2);
}

////////////////////////////////////////////////////////////////////////////////
// sync_wait() triggers a call to sync() and blocks until that call
// completes. If there is a sync() in progress, it waits for that sync()
// to complete and then launches another sync so that a full cycle is
// completed before returning.
// Any number of users of this module can call sync_wait() concurrently.
////////////////////////////////////////////////////////////////////////////////
void module::sync_wait() {
    report("[MODULE] sync_wait()", 3);
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
    report("[MODULE] sync_wait() done", 3);
}

////////////////////////////////////////////////////////////////////////////////
// sync_now() triggers a call to sync() and returns immediately. If
// there is a sync() in progress, another sync() call is scheduled to
// begin immediately upon return of the current call.
////////////////////////////////////////////////////////////////////////////////
void module::sync_now() {
    report("[MODULE] sync_now()", 3);
    std::unique_lock<std::mutex> lck(mtx);
    skip_wait = true;
    cv_mt.notify_all();
    report("[MODULE] sync_now() done", 4);
}

////////////////////////////////////////////////////////////////////////////////
// heart_beat_wait() requests an immediate heart beat and waits for one second
// or until it completes, whichever is faster. Any number of users of this
// module can call heart_beat_wait() concurrently.
////////////////////////////////////////////////////////////////////////////////
void module::heart_beat_wait() {
    report("[MODULE] heart_beat_wait()", 3);
    std::unique_lock<std::mutex> lck(mtx);
    if (!heart_beat_requested) heart_beat_request_time = now_floor();
    heart_beat_requested = true;
    cv_mt.notify_all();
    // Timeout after 1 second.
    cv_wt.wait_for(lck, duration(1));
    report("[MODULE] heart_beat_wait() done", 3);
}

////////////////////////////////////////////////////////////////////////////////
// heart_beat_now() requests an immediate heart beat and then returns without
// waiting.
////////////////////////////////////////////////////////////////////////////////
void module::heart_beat_now() {
    report("[MODULE] heart_beat_now()", 3);
    std::unique_lock<std::mutex> lck(mtx);
    if (!heart_beat_requested) heart_beat_request_time = now_floor();
    heart_beat_requested = true;
    cv_mt.notify_all();
    report("[MODULE] heart_beat_now() done", 3);
}

////////////////////////////////////////////////////////////////////////////////
// Answers: How long has the heart beat been missing for? If no heart beat
// was requested, returns zero duration.
////////////////////////////////////////////////////////////////////////////////
module::duration module::heart_beat_missed() {
    report("[MODULE] heart_beat_missed()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    duration res = duration(0);
    if (!heart_beat_requested) {
        heart_beat_request_time = now_floor();
    }
    else {
        time_point nf = now_floor();
        if (nf > heart_beat_request_time + duration(1))
            res = nf - heart_beat_request_time - duration(1);
    }
    heart_beat_requested = true;
    cv_mt.notify_all();
    char report_str[64];
    snprintf(report_str, 64, "[MODULE] heart_beat_missed(): %ld", res.count());
    if (res > duration(5))
        report(report_str, 3);
    else
        report(report_str, 4);
    return res;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
module::module(bool automatic, int update_frequency) {
    strncpy(name, "MODULE [ empty ]", 64);
    report("[MODULE] constructor", 3);
    this->automatic = automatic;
    this->update_frequency = update_frequency;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::enable() {
    thread = std::thread(&management_thread, this);
    sync_wait();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::disable() {
    report("[MODULE] disable()", 3);
    std::unique_lock<std::mutex> lck(mtx);
    // Wait for the next iteration to finish
    done = true;
    skip_wait = true;
    while (!mt_exit) {
        cv_mt.notify_all();
        // Timeout after 5 seconds and verify that sync() hasn't
        // completed yet.
        cv_wt.wait_for(lck, duration(5));
    };
    thread.join();
    report("[MODULE] disable() done", 3);
}

////////////////////////////////////////////////////////////////////////////////
// Report an event.
////////////////////////////////////////////////////////////////////////////////
void module::report(char* text, int verbosity, bool log) {
    std::unique_lock<std::mutex> lck(log_mtx);
    char time_str[64];
    time_t time = sc::to_time_t(now_floor());
    strftime(time_str, 64, "%c", std::localtime(&time));

    if (log) {
        if (log_file[0]) {
            FILE* f = fopen(log_file, "a");
            if (f) {
                fprintf(f, "%s ; %s ; %s\n", time_str, name, text);
                fflush(f);
                fclose(f);
            } else {
                printf("Failed to open log file (%d): ", errno);
                fflush(stdout);
                printf("%s\n", log_file);
                fflush(stdout);
            }
        }
        if (verbosity_limit >= verbosity) {
            printf("%s ; %s ; log <- %s\n", time_str, name, text);
            fflush(stdout);
        }
    }
    else if (verbosity_limit >= verbosity) {
        printf("%s ; %s ; %s\n", time_str, name, text);
        fflush(stdout);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::report(const char* text, int verbosity, bool log) {
    char str[256];
    strncpy(str, text, 256);
    report(str, verbosity, log);
}

////////////////////////////////////////////////////////////////////////////////
// Scan for events.
////////////////////////////////////////////////////////////////////////////////
module::time_point module::scan_report(char* text) {
    std::unique_lock<std::mutex> lck(log_mtx);

    char looking_for[512], input_time_str[512], input_line[512];
    snprintf(looking_for, 512, "%s ; %s", name, text);


    time_point found_time = now_floor();

    if (log_file[0]) {
        FILE* f = fopen(log_file, "r");
        if (f) {
            while (true) {
                if (fscanf(f, "%512[^;]", input_time_str) == EOF) break;
                if (fscanf(f, "%512[^\n] ", input_line) == EOF) break;
                if (nullptr != strstr(input_line, looking_for)) {
                    // Found a match. Get the time.
                    struct tm time;
                    strptime(input_time_str, "%c", &time);
                    time_t tt = mktime(&time);
                    found_time = std::chrono::time_point_cast<std::chrono::seconds>(sc::from_time_t(tt));
                }
            }
            fclose(f);
        }
    }

    return found_time;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
module::time_point module::scan_report(const char* text) {
    char str[256];
    strncpy(str, text, 256);
    return scan_report(str);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::set_name(char* name) {
    std::unique_lock<std::mutex> lck(log_mtx);
    strncpy(this->name, name, 64);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::set_verbosity(int verbosity) {
    std::unique_lock<std::mutex> lck(log_mtx);
    verbosity_limit = verbosity;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::set_log_file(char* log_file) {
    std::unique_lock<std::mutex> lck(log_mtx);
    strncpy(module::log_file, log_file, 256);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
module::time_point module::now_ceil() {
    return std::chrono::ceil<duration>(sc::now());
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
module::time_point module::now_floor() {
    return std::chrono::floor<duration>(sc::now());
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::notify_listeners() {
    std::unique_lock<std::mutex> lck(listeners_mtx);
    for (auto iter = listeners.begin(); iter != listeners.end(); iter++)
        (*iter)->sync_now();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::listen(module* m) {
    std::unique_lock<std::mutex> lck(listeners_mtx);
    listeners.insert(m);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void module::unlisten(module* m) {
    std::unique_lock<std::mutex> lck(listeners_mtx);
    listeners.erase(m);
}
