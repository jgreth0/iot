
#ifndef _MODULE_H_
#define _MODULE_H_

#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>

class module {
    public:
        using sc = std::chrono::system_clock;
        using duration = std::chrono::seconds;
        using time_point = std::chrono::time_point<sc, duration>;

    private:
        std::mutex mtx;
        std::condition_variable cv_mt, cv_wt;
        bool done = false, skip_wait = false, automatic = true;
        int update_frequency = 15;
        uint64_t sync_start_count = 0, sync_finish_count = 0;
        std::thread thread;
        static void management_thread(module* m);
        // Logging
        static char log_file[256];
        static std::mutex log_mtx;

        ////////////////////////////////////////////////////////////////////////
        // App verbosity:
        // 0 - OFF, only errors are reported.
        // 1 - BASIC (default), basic startup and teardown events.
        // 2 - LOW, adds log events (copied to stdout)
        // 3 - MEDIUM, adds info about device thread start/stop
        // 4 - FULL, currently same as MEDIUM
        // 5 - DEBUG, includes function stop/start infos.
        ////////////////////////////////////////////////////////////////////////
        static int VERBOSITY;
    protected:
        // The derived class implements the sync capability and any required
        // setters and getters. The derived class is expected to protect its own
        // state against race conditions.
        virtual void sync(bool last = false);
        virtual void get_name(char* name);
        // Report an event.
        void report(char* text, int verbosity = 2, bool log = false);
        void report(const char* text, int verbosity = 2, bool log = false);
    public:
        // sync_wait() triggers a call to sync() and blocks until that call
        // completes. If there is a sync() in progress, it waits for that sync()
        // to complete and then launches another sync so that a full cycle is
        // completed before returning.
        // Any number of users of this module can call sync_wait() concurrently.
        void sync_wait();
        // sync_now() triggers a call to sync() and returns immediately. If
        // there is a sync() in progress, another sync() call is scheduled to
        // begin immediately upon return of the current call.
        void sync_now();
        module(bool automatic = true, int update_frequency = 15);
        ~module();
        time_point now();
        static void set_verbosity(int VERBOSITY);
        static void set_log_file(char* log_file);
};

#endif
