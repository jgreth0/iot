
#ifndef _KASA_H_
#define _KASA_H_

#include "../module/module.hpp"
#include <thread>
#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <condition_variable>

class kasa : public module {

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
        char addr[64], name[256];
        int res = UNKNOWN, tgt = UNCHANGED;
        std::mutex mtx;

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
        // Attempt to switch a device on or off.
        // Returns the post-switch state.
        // If the target state is something other than ON or OFF, the state is
        // not changed but the current state is still queried and returned.
        ////////////////////////////////////////////////////////////////////////
        int sync_device(int tgt);

    protected:
        void sync(bool last = false);
        void get_name(char* name);

    public:
        ////////////////////////////////////////////////////////////////////////
        // Get the most recent state for a deivce.
        ////////////////////////////////////////////////////////////////////////
        int get_status();

        ////////////////////////////////////////////////////////////////////////
        // Set the target state for a deivce.
        ///////////////////////////////////////////////////////////////////////////
        void set_target(int tgt);

        ///////////////////////////////////////////////////////////////////////////
        // Start the KASA runtime.
        ///////////////////////////////////////////////////////////////////////////
        kasa(char* name, char* addr);
};

#endif
