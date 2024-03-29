
#ifndef _UNIT_H_
#define _UNIT_H_

#include <chrono>
#include <mutex>
#include <vector>
#include <cstring>
#include <csignal>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class unit {
public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    using sc = std::chrono::system_clock;
    using duration = std::chrono::seconds;
    using time_point = std::chrono::time_point<sc, duration>;

private:
    ////////////////////////////////////////////////////////////////////////////
    // Report
    ////////////////////////////////////////////////////////////////////////////
    char name[64];

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    static char log_file[256];
    static std::mutex log_mtx;
    static int verbosity_limit;

protected:
    ////////////////////////////////////////////////////////////////////////////
    // Report
    ////////////////////////////////////////////////////////////////////////////
    void set_name(char* name);

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void report(char* text, int verbosity = 2, bool log = false);
    void report(const char* text, int verbosity = 2, bool log = false);

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    time_point scan_report(char* text);
    time_point scan_report(const char* text);

public:
    unit();

    ////////////////////////////////////////////////////////////////////////////
    // Report
    ////////////////////////////////////////////////////////////////////////////
    static void set_verbosity(int verbosity);

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    static void set_log_file(char* log_file);

    ////////////////////////////////////////////////////////////////////////////
    // Utils
    ////////////////////////////////////////////////////////////////////////////
    time_point now_ceil();

    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    time_point now_floor();
};

#endif
