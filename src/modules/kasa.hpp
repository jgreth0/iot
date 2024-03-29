
#ifndef _KASA_H_
#define _KASA_H_

#include "module.hpp"
#include "icmp_helper.hpp"
#include <thread>
#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <condition_variable>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class kasa : public module {
public:
    ////////////////////////////////////////////////////////////////////////////
    // Constants - Device States
    ////////////////////////////////////////////////////////////////////////////
    static inline const int UNCHANGED = 0;
    static inline const int ON = 1;
    static inline const int OFF = 2;
    static inline const int ERROR = 3;
    static inline const int UNKNOWN = 4;
    static inline const char* const STATES[] =
        {"UNCHANGED", "ON", "OFF", "ERROR", "UNKNOWN"};

private:
    ////////////////////////////////////////////////////////////////////////////
    // Configuration - only written by the constructor.
    ////////////////////////////////////////////////////////////////////////////
    char addr[64];
    icmp_helper ping;
    duration cooldown, error_cooldown;

    ////////////////////////////////////////////////////////////////////////////
    // Status information - updated on every sync()
    // Access must be protected by mutex.
    ////////////////////////////////////////////////////////////////////////////
    time_point toggle_time = now_floor();
    time_point last_time_on = now_floor(), last_time_off = now_floor();
    bool recent_error = false;
    time_point connect_time;
    int res = UNKNOWN, tgt = UNCHANGED;
    int res_brightness = 0;
    int start_brightness = 0, end_brightness = 0;
    int res_power_mw = -1, res_total_wh = -1;
    time_point start_time = now_floor(), end_time = now_floor();
    std::mutex mtx;

    ////////////////////////////////////////////////////////////////////////////
    // IO context - A single connection is used multiple times.
    ////////////////////////////////////////////////////////////////////////////
    int sock = -1;

    ////////////////////////////////////////////////////////////////////////////
    // Encode a command to a KASA device. Requires an extra 4 bytes in the
    // input buffer to store the longer result. Returns the length of the
    // encoded command. The operation is done in-place in the data buffer.
    ////////////////////////////////////////////////////////////////////////////
    int encode(char* data);

    ////////////////////////////////////////////////////////////////////////////
    // Decode a response from a KASA device. The operation is done in-place in
    // the data buffer.
    ////////////////////////////////////////////////////////////////////////////
    bool decode(char* data, int len);

protected:
    ////////////////////////////////////////////////////////////////////////////
    // The c_str in 'data' is sent to the kasa device. The response is written
    // into 'data'.
    ////////////////////////////////////////////////////////////////////////////
    void send_recv(char* data, int data_len, bool last);

    ////////////////////////////////////////////////////////////////////////////
    // Attempt to switch a device on or off.
    // Returns the post-switch state.
    //
    // If the target state is something other than ON or OFF, the state is
    // not changed but the current state is still queried and returned.
    //
    // If an attempt is made to update the device state (successful or not),
    // toggle_time is updated. Subsequent state changes are suppressed until the
    // cooldown period has completed. This ensures that the device is not
    // damaged by excessive/quick toggling on and off.
    ////////////////////////////////////////////////////////////////////////////
    void sync_device(int tgt, int tgt_brightness, int* res, int* res_brightness,
        int* res_power_mw, int* res_total_wh, bool last);

    ////////////////////////////////////////////////////////////////////////////
    // Write the target state to the device and query the current state.
    // If the device state has changed, note the change in the log.
    // Once a new target state is successfully applied, the target state becomes
    // UNCHANGED.
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false);

public:
    ////////////////////////////////////////////////////////////////////////////
    // Returns the state from the most recent device query.
    // Call sync_wait() before this to ensure that the returned value is
    // up-to-date.
    ////////////////////////////////////////////////////////////////////////////
    int get_status();
    int get_power_mw();
    int get_total_wh();

    ////////////////////////////////////////////////////////////////////////////
    // Gets the target device state which will be applied on the next sync.
    ////////////////////////////////////////////////////////////////////////////
    int get_target();

    ////////////////////////////////////////////////////////////////////////////
    // Returns the state from the most recent device query.
    // Call sync_wait() before this to ensure that the returned value is
    // up-to-date.
    ////////////////////////////////////////////////////////////////////////////
    int get_brightness_status();

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last in the 'ON' state
    // If the device is currently in the 'ON' state, returns now_floor().
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_on();

    ////////////////////////////////////////////////////////////////////////////
    // When was the device last in the 'OFF' state
    // If the device is currently in the 'OFF' state, returns now_floor().
    ////////////////////////////////////////////////////////////////////////////
    time_point get_last_time_off();

    ////////////////////////////////////////////////////////////////////////////
    // Sets the target device state which will be applied promptly.
    ////////////////////////////////////////////////////////////////////////////
    void set_target(int tgt);

    ////////////////////////////////////////////////////////////////////////////
    // Sets the target device state which will be applied promptly.
    ////////////////////////////////////////////////////////////////////////////
    void set_brightness_target(int start_brightness, int end_brightness,
                               time_point start_time, time_point end_time);

    ////////////////////////////////////////////////////////////////////////////
    // Start the KASA runtime.
    ////////////////////////////////////////////////////////////////////////////
    kasa(char* name, char* addr, int update_frequency = 1,
        int cooldown = 5, int error_cooldown = 15);
    kasa(const char* name, const char* addr, int update_frequency = 1,
        int cooldown = 5, int error_cooldown = 15);
};

#endif
