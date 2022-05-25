
#include <thread>
#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <condition_variable>

class kasa {

    using sc = std::chrono::system_clock;
    using d5 = std::chrono::duration<long long, std::ratio<15,1>>;
    using tp5 = std::chrono::time_point<sc, d5>;

    public:
        ////////////////////////////////////////////////////////////////////////
        // Constants
        ////////////////////////////////////////////////////////////////////////
        static const int UNCHANGED = 0;
        static const int ON = 1;
        static const int OFF = 2;
        static const int ERROR = 3;
        static const int UNKNOWN = 4;
        static const char* STATES[];

    private:
        ////////////////////////////////////////////////////////////////////////
        // Local state is accessed exclusively by the management thread.
        // Accesses are not required to be protected by mutex.
        ////////////////////////////////////////////////////////////////////////
        std::thread thread = std::thread(&dmt, this);
        char addr[64], name[64], log_file[256];
        int res = UNKNOWN, tgt = UNCHANGED;
        bool fs = false, done = false;

        static std::mutex log_mtx;
        static std::map<std::string,kasa> device_map;

        ////////////////////////////////////////////////////////////////////////
        // App verbosity:
        // 0 - OFF, only errors are reported.
        // 1 - BASIC (default), basic startup and teardown events.
        // 2 - LOW, adds log events (copied to stdout)
        // 3 - MEDIUM, adds info about device thread start/stop
        // 4 - FULL, currently same as MEDIUM
        // 5 - DEBUG, includes function stop/start infos.
        ////////////////////////////////////////////////////////////////////////
        int VERBOSITY = 1;

        ////////////////////////////////////////////////////////////////////////
        // Global state is accessed by multiple threads. All accesses are
        // protected by mutex.
        ////////////////////////////////////////////////////////////////////////

        std::mutex mtx;
        std::condition_variable mt_cv;
        std::condition_variable fs_cv;

        int res_x = UNKNOWN, tgt_x = UNCHANGED;
        bool fs_x = false, done_x = false;

        ////////////////////////////////////////////////////////////////////////
        // Encode a command to a KASA device. Requires an extra 4 bytes in the
        // input buffer to store the longer result. Returns the length of the
        // encoded command.
        // - This version was written by John Greth.
        // - This was created by reverse-engineering the decode() function in
        //   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
        //   by George Georgovassilis. George credits Thomas Baust for providing
        //   the KASA device encoding scheme.
        ////////////////////////////////////////////////////////////////////////
        int encode(char* data);

        ////////////////////////////////////////////////////////////////////////
        // Decode a response from a KASA device.
        // - This version was written by John Greth.
        // - This was created by reverse-engineering the decode() function in
        //   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
        //   by George Georgovassilis. George credits Thomas Baust for providing
        //   the KASA device encoding scheme.
        ////////////////////////////////////////////////////////////////////////
        void decode(char* data, int len);

        ////////////////////////////////////////////////////////////////////////
        // The c_str in 'data' is sent to the kasa device at the given IPv4
        // address. The response is written into 'data'.
        ////////////////////////////////////////////////////////////////////////
        void send_recv(char* data, int data_len);

        ////////////////////////////////////////////////////////////////////////
        // Print to the log file.
        ////////////////////////////////////////////////////////////////////////
        void log(char* txt);

        ////////////////////////////////////////////////////////////////////////
        // Attempt to switch a device on or off.
        // Returns the post-switch state.
        // If the target state is something other than ON or OFF, the state is
        // not changed but the current state is still queried and returned.
        ////////////////////////////////////////////////////////////////////////
        int sync_device();

        ////////////////////////////////////////////////////////////////////////
        // Synchronize the local thread state with the global state.
        ////////////////////////////////////////////////////////////////////////
        bool sync_state(bool checked, tp5 tm);

        ////////////////////////////////////////////////////////////////////////
        // Each device has it's own thread. This thread is the only one which
        // can communicate with that device. The control thread communicates
        // with the device threads through global state synchronizations
        // (above).
        ////////////////////////////////////////////////////////////////////////
        void device_management_thread();

        static void dmt(kasa* obj) {
            obj->device_management_thread();
        }

    public:
        ////////////////////////////////////////////////////////////////////////
        // Get the most recent state for a deivce.
        ////////////////////////////////////////////////////////////////////////
        int get_status();

        ////////////////////////////////////////////////////////////////////////
        // Set the target state for a deivce.
        ///////////////////////////////////////////////////////////////////////////
        void set_target(int tgt);

        ////////////////////////////////////////////////////////////////////////
        // Flush the target states to each device and refresh the observed
        // states.  This function blocks until the operation is complete.
        // get_status() returns a fresh value after force_sync() returns.
        ///////////////////////////////////////////////////////////////////////////
        void force_sync();

        ///////////////////////////////////////////////////////////////////////////
        // Start the KASA runtime.
        ///////////////////////////////////////////////////////////////////////////
        kasa(char* log_file, char* name, char* addr, int verbosity = 1);

        ///////////////////////////////////////////////////////////////////////////
        // Stop the KASA runtime.
        ///////////////////////////////////////////////////////////////////////////
        ~kasa();

        ///////////////////////////////////////////////////////////////////////////
        // Start the KASA runtime. Reads the config file and initializes each
        // device.
        ///////////////////////////////////////////////////////////////////////////
        static void load_conf(char* log_file, char* conf_file, int verbosity = 1);
        static void stop_all();

        ///////////////////////////////////////////////////////////////////////////
        // Look up the named device and perform the requested operation. This
        // works for devices initialized with load_conf()
        ///////////////////////////////////////////////////////////////////////////
        static int get_status(char* name);
        static void set_target(char* name, int tgt);
        static void force_sync(char* name);
        static void force_sync_all();
};
