
#include "unit.hpp"
#include <ctime>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
std::mutex unit::log_mtx;
int unit::verbosity_limit = 3;
char unit::log_file[256] = "";

////////////////////////////////////////////////////////////////////////////////
// Report an event.
////////////////////////////////////////////////////////////////////////////////
void unit::report(char* text, int verbosity, bool log) {
    std::unique_lock<std::mutex> lck(log_mtx);
    char time_str[64];
    time_t time = sc::to_time_t(now_floor());
    strftime(time_str, 64, "%c", std::localtime(&time));

    if (verbosity_limit >= verbosity) {
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
            printf("%s ; %s ; log <- %s\n", time_str, name, text);
            fflush(stdout);
        }
        else {
            printf("%s ; %s ; %s\n", time_str, name, text);
            fflush(stdout);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void unit::report(const char* text, int verbosity, bool log) {
    char str[256];
    strncpy(str, text, 256);
    report(str, verbosity, log);
}

////////////////////////////////////////////////////////////////////////////////
// Scan for events.
////////////////////////////////////////////////////////////////////////////////
unit::time_point unit::scan_report(char* text) {
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
unit::time_point unit::scan_report(const char* text) {
    char str[256];
    strncpy(str, text, 256);
    return scan_report(str);
}

unit::unit() {
    strncpy(name, "UNIT [ empty ]", 64);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void unit::set_name(char* name) {
    std::unique_lock<std::mutex> lck(log_mtx);
    strncpy(this->name, name, 64);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void unit::set_verbosity(int verbosity) {
    std::unique_lock<std::mutex> lck(log_mtx);
    verbosity_limit = verbosity;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void unit::set_log_file(char* log_file) {
    std::unique_lock<std::mutex> lck(log_mtx);
    strncpy(unit::log_file, log_file, 256);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
unit::time_point unit::now_ceil() {
    return std::chrono::ceil<duration>(sc::now());
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
unit::time_point unit::now_floor() {
    return std::chrono::floor<duration>(sc::now());
}
