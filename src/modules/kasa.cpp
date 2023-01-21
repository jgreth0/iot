
#include "kasa.hpp"
#include <stdio.h>
#include <cstring>
#include <ctime>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////
// Encode a command to a KASA device. Requires an extra 4 bytes in the
// input buffer to store the longer result. Returns the length of the
// encoded command. The operation is done in-place in the data buffer.
// - This version was written by John Greth.
// - This was created by reverse-engineering the decode() function in
//   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
//   by George Georgovassilis. George credits Thomas Baust for providing
//   the KASA device encoding scheme.
////////////////////////////////////////////////////////////////////////////////
int kasa::encode(char* data) {
    char temp[5];
    for (int j = 0; j < 5; j++) temp[j] = data[j];
    data[3] = 171;
    int len = 0;
    while (temp[0]) {
        data[len+4] = data[len+3] ^ temp[0];
        for (int j = 0; j < 4; j++) temp[j] = temp[j+1];
        temp[4] = data[4 + ++len];
    }
    for (int i = 0; i < 4; i++) data[3-i] = (len >> (8*i)) & 255;
    return len+4;
}

////////////////////////////////////////////////////////////////////////////////
// Decode a response from a KASA device. The operation is done in-place in the
// data buffer.
// - This version was written by John Greth.
// - This was created by reverse-engineering the decode() function in
//   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
//   by George Georgovassilis. George credits Thomas Baust for providing
//   the KASA device encoding scheme.
////////////////////////////////////////////////////////////////////////////////
void kasa::decode(char* data, int len) {
    int msg_len = 0;
    bool ret = true;
    for (int i = 0; i < 4; i++) msg_len = (msg_len << 8) + data[i];
    if (msg_len < len) len = msg_len;
    if (len < 4) len = 4; // For safety in error cases.
    data[3] = 171;
    for (int i = 0; i < len-4; i++) data[i] = data[i+3] ^ data[i+4];
    data[len-4] = 0;
}

////////////////////////////////////////////////////////////////////////////////
// The c_str in 'data' is sent to the kasa device. The response is written into
// 'data'.
////////////////////////////////////////////////////////////////////////////////
void kasa::send_recv(char* data, int data_len, bool last) {
    char report_str[1024];

    if (last) {
        close(sock);
        data[0] = '\0';
        return;
    }

    // Wait a random amount, up to 200ms, to avoid bursts.
    usleep(1000 * (rand() % 250));

    bool error_detected = false;

    // Send the encoded command.
    sprintf(report_str, "Message sent: %s", data);
    report(report_str, 6);
    int encode_len = encode(data);

    if (sock == -1) {
        error_detected = true;
    } else if (encode_len != write(sock, data, encode_len)) {
        error_detected = true;
    } else {
        // Wait for a response.
        // Timeout after 400ms. State checks take <100ms, but compound
        // commands (set and check) may take a bit longer.
        struct pollfd pfd = {.fd = sock, .events = POLLIN};
        poll(&pfd, 1, 400);
        if (pfd.revents != POLLIN)
            error_detected = true;
    }

    if (error_detected) {
        if (connect_time + error_cooldown <= now_floor()) {
            error_detected = false;

            report("Connection error. Retrying...", 3);

            close(sock);

            sock = socket(AF_INET, SOCK_STREAM, 0);
            // Set non-blocking so that timeouts can be enforced.
            fcntl(sock, F_SETFL, O_NONBLOCK);

            // Attempt a connection to the device.
            struct sockaddr_in sock_addr =
                {.sin_family = AF_INET, .sin_port = htons(9999)};
            inet_pton(AF_INET, addr, &sock_addr.sin_addr);
            connect(sock, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

            // Wait for the connection to be established.
            // Timeout after 100ms.
            struct pollfd pfd = {.fd = sock, .events = POLLOUT};
            poll(&pfd, 1, 100);
            if (pfd.revents != POLLOUT)
                error_detected = true;

            if (encode_len != write(sock, data, encode_len))
                error_detected = true;

            connect_time = now_floor();

            // Wait for a response.
            // Timeout after 400ms. State checks take <100ms, but compound
            // commands (set and check) may take a bit longer.
            pfd = {.fd = sock, .events = POLLIN};
            poll(&pfd, 1, 400);
            if (pfd.revents != POLLIN)
                error_detected = true;

            if (error_detected) {
                report("Connection error was not resolved. Returning error.", 3);
            }

        } else {
            report("Connection error. Waiting...", 4);
        }
    }

    if (!error_detected) {
        // The socket has data to receive.
        // Fetch and decode it.
        report("decoding response from the device", 5);
        decode(data, recv(sock, data, data_len, 0));
        sprintf(report_str, "Message received: %s", data);
        report(report_str, 6);
    }
    else {
        data[0] = '\0';
    }
}

////////////////////////////////////////////////////////////////////////////////
// Attempt to switch a device on or off.
// Returns the post-switch state.
//
// If the target state is something other than ON or OFF, the state is
// not changed but the current state is still queried and returned.
//
// If an attempt is made to update the device state (successful or not),
// toggle_time is updated. Subsequent state changes are suppressed until the
// cooldown period has completed. This ensures that the device is not damaged by
// excessive/quick toggling on and off.
////////////////////////////////////////////////////////////////////////////////
void kasa::sync_device(int tgt, int tgt_brightness, int* res, int* res_brightness,
        int* res_power_mw, int* res_total_wh, bool last) {
    *res_brightness = 100;
    report("sync_device() called.", 5);
    char data[4096];

    if (tgt_brightness) {
        snprintf(data, 4096, "{\"smartlife.iot.dimmer\":{\"set_brightness\":{\"brightness\":%d}}}", tgt_brightness);
        send_recv(data, 4096, false);
    }

    if (now_floor() - toggle_time < cooldown)
        tgt = UNCHANGED;
    if (tgt == ON) {
        strncpy(data, "{\"emeter\":{\"get_realtime\":null},\"system\":{\"set_relay_state\":{\"state\":1},\"get_sysinfo\":null}}", 4096);
        toggle_time = now_floor();
    } else if (tgt == OFF) {
        strncpy(data, "{\"emeter\":{\"get_realtime\":null},\"system\":{\"set_relay_state\":{\"state\":0},\"get_sysinfo\":null}}", 4096);
        toggle_time = now_floor();
    } else
        strncpy(data, "{\"emeter\":{\"get_realtime\":null},\"system\":{\"get_sysinfo\":null}}", 4096);

    send_recv(data, 4096, last);

    if (NULL != strstr(data, "\"relay_state\":1")) *res = ON;
    else if (NULL != strstr(data, "\"relay_state\":0")) *res = OFF;
    else *res = ERROR;

    char* res_str = strstr(data, "\"brightness\":");
    if (res_str) {
        res_str += 13;
        *res_brightness = atoi(res_str);
    }
    else *res_brightness = 0;

    res_str = strstr(data, "\"power_mw\":");
    if (res_str) {
        res_str += 11;
        *res_power_mw = atoi(res_str);
    }
    else *res_power_mw = -1;

    res_str = strstr(data, "\"total_wh\":");
    if (res_str) {
        res_str += 11;
        *res_total_wh = atoi(res_str);
    }
    else *res_total_wh = -1;

    report("sync_device() complete.", 5);
}

////////////////////////////////////////////////////////////////////////////////
// Write the target state to the device and query the current state.
// If the device state has changed, note the change in the log.
// Once a new target state is successfully applied, the target state becomes
// UNCHANGED.
////////////////////////////////////////////////////////////////////////////////
void kasa::sync(bool last) {
    std::unique_lock<std::mutex> lck(mtx);
    int tgt = this->tgt;
    int tgt_brightness = end_brightness;
    time_point current_time = now_floor();
    if (current_time <= start_time) {
        tgt_brightness = start_brightness;
    } else if (current_time >= end_time) {
        tgt_brightness = end_brightness;
    } else {
        tgt_brightness = start_brightness +
            (((end_brightness - start_brightness) *
            (current_time - start_time)) /
            (end_time - start_time));
    }
    if (tgt_brightness == res_brightness || res_brightness == 0)
        tgt_brightness = 0;
    if (!recent_error) {
        if (res == ON ) last_time_on  = now_floor();
        if (res == OFF) last_time_off = now_floor();
    }
    lck.unlock();
    int res, res_brightness;
    int res_power_mw, res_total_wh;
    sync_device(tgt, tgt_brightness, &res, &res_brightness,
        &res_power_mw, &res_total_wh, last);
    lck.lock();
    if (res == ON ) {
        last_time_on  = now_floor();
        recent_error = false;
    }
    if (res == OFF) {
        last_time_off = now_floor();
        recent_error = false;
    }
    if (res == ERROR) recent_error = true;

    if (last) res = UNKNOWN;

    if (res_brightness == end_brightness && current_time >= end_time) {
        if (end_brightness == 100) {
            end_brightness = 0;
            start_brightness = 0;
        } else {
            end_brightness = 100;
        }
    }
    this->res_brightness = res_brightness;
    if (res_power_mw != -1) this->res_power_mw = res_power_mw;
    if (res_total_wh != -1) this->res_total_wh = res_total_wh;
    if (res == this->tgt) this->tgt = UNCHANGED;
    if (this->res == res) return;
    // The value has changed.
    // Don't acknowledge an error unless it has been persistent.
    if ((res == ERROR) &&
        (error_cooldown > current_time - last_time_on) &&
        (error_cooldown > current_time - last_time_off)) return;
    int res_prev = this->res;
    this->res = res;
    lck.unlock();
    char report_str[256];
    sprintf(report_str, "state: %s", STATES[res_prev]);
    report(report_str, 2, true);
    if (this->res_total_wh != -1) {
        sprintf(report_str, "power: %d", res_total_wh);
        report(report_str, 2, true);
    }
    sprintf(report_str, "state: %s", STATES[res]);
    report(report_str, 2, true);
    notify_listeners();
}

////////////////////////////////////////////////////////////////////////////////
// When was the device last in the 'ON' state
// If the device is currently in the 'ON' state, returns now_floor().
////////////////////////////////////////////////////////////////////////////////
kasa::time_point kasa::get_last_time_on() {
    report("get_last_time_on()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    time_point t = last_time_on;
    // The last update may have been a few seconds ago. Manually override to
    // now_floor().
    if (res == ON) t = now_floor();
    lck.unlock();
    report("get_last_time_on() done", 5);
    return t;
}

////////////////////////////////////////////////////////////////////////////////
// When was the device last in the 'OFF' state
// If the device is currently in the 'OFF' state, returns now_floor().
////////////////////////////////////////////////////////////////////////////////
kasa::time_point kasa::get_last_time_off() {
    report("get_last_time_off()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    time_point t = last_time_off;
    // The last update may have been a few seconds ago. Manually override to
    // now_floor().
    if (res == OFF) t = now_floor();
    lck.unlock();
    report("get_last_time_off() done", 5);
    return t;
}

////////////////////////////////////////////////////////////////////////////////
// Returns the state from the most recent device query.
// Call sync_wait() before this to ensure that the returned value is up-to-date.
////////////////////////////////////////////////////////////////////////////////
int kasa::get_status() {
    report("get_status()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    int res = this->res;
    lck.unlock();
    report("get_status() done", 5);
    return res;
}

int kasa::get_power_mw() {
    report("get_power_mw()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    int res = this->res_power_mw;
    lck.unlock();
    report("get_power_mw() done", 5);
    return res;
}

int kasa::get_total_wh() {
    report("get_total_wh()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    int res = this->res_total_wh;
    lck.unlock();
    report("get_total_wh() done", 5);
    return res;
}

////////////////////////////////////////////////////////////////////////////////
// Gets the target device state which will be applied on the next sync.
////////////////////////////////////////////////////////////////////////////////
int kasa::get_target() {
    report("get_target()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    int tgt = this->tgt;
    lck.unlock();
    report("get_target() done", 5);
    return tgt;
}

////////////////////////////////////////////////////////////////////////////////
// Sets the target device state which will be applied promptly.
////////////////////////////////////////////////////////////////////////////////
void kasa::set_target(int tgt) {
    char report_str[256];
    sprintf(report_str, "set_target(%s)", STATES[tgt]);
    report(report_str, 3);
    std::unique_lock<std::mutex> lck(mtx);
    this->tgt = tgt;
    lck.unlock();
    sync_now();
    sprintf(report_str, "set_target(%s) done", STATES[tgt]);
    report(report_str, 4);
}

////////////////////////////////////////////////////////////////////////////////
// Returns the state from the most recent device query.
// Call sync_wait() before this to ensure that the returned value is up-to-date.
////////////////////////////////////////////////////////////////////////////////
int kasa::get_brightness_status() {
    report("get_brightness_status()", 5);
    std::unique_lock<std::mutex> lck(mtx);
    int res_brightness = this->res_brightness;
    lck.unlock();
    report("get_brightness_status() done", 5);
    return res_brightness;
}

////////////////////////////////////////////////////////////////////////////////
// Sets the target device state which will be applied promptly.
////////////////////////////////////////////////////////////////////////////////
void kasa::set_brightness_target(int start_brightness, int end_brightness,
                                 time_point start_time, time_point end_time) {
    char report_str[256];
    sprintf(report_str, "set_brightness_target()");
    report(report_str, 3);
    std::unique_lock<std::mutex> lck(mtx);
    this->start_brightness = start_brightness;
    this->end_brightness = end_brightness;
    this->start_time = start_time;
    this->end_time = end_time;
    lck.unlock();
    sync_now();
    sprintf(report_str, "set_brightness_target() done");
    report(report_str, 4);
}

////////////////////////////////////////////////////////////////////////////////
// Start the KASA runtime.
////////////////////////////////////////////////////////////////////////////////
kasa::kasa(char* name, char* addr, int update_frequency,
        int cooldown, int error_cooldown) : module(true, update_frequency) {
    char name_full[64];
    snprintf(name_full, 64, "KASA [ %s @ %s ]", name, addr);
    set_name(name_full);
    strncpy(this->addr, addr, 64);
    this->cooldown = duration(cooldown);
    this->error_cooldown = duration(error_cooldown);
    connect_time = now_floor() - this->error_cooldown;

    char report_str[256], time_str[64];
    sprintf(report_str, "state: %s", STATES[ON]);
    last_time_on = scan_report(report_str);
    sprintf(report_str, "state: %s", STATES[OFF]);
    last_time_off = scan_report(report_str);

    time_t time = sc::to_time_t(last_time_on);
    strftime(time_str, 64, "%c", std::localtime(&time));
    sprintf(report_str, "init last_time_on: %s", time_str);
    report(report_str, 3);

    time = sc::to_time_t(last_time_off);
    strftime(time_str, 64, "%c", std::localtime(&time));
    sprintf(report_str, "init last_time_off: %s", time_str);
    report(report_str, 3);

    report("constructor done", 3);
}

kasa::kasa(const char* name, const char* addr, int update_frequency,
        int cooldown, int error_cooldown) : module(true, update_frequency) {
    char name_full[64];
    snprintf(name_full, 64, "KASA [ %s @ %s ]", name, addr);
    set_name(name_full);
    strncpy(this->addr, addr, 64);
    this->cooldown = duration(cooldown);
    this->error_cooldown = duration(error_cooldown);
    connect_time = now_floor() - this->error_cooldown;

    char report_str[256], time_str[64];
    sprintf(report_str, "state: %s", STATES[ON]);
    last_time_on = scan_report(report_str);
    sprintf(report_str, "state: %s", STATES[OFF]);
    last_time_off = scan_report(report_str);

    time_t time = sc::to_time_t(last_time_on);
    strftime(time_str, 64, "%c", std::localtime(&time));
    sprintf(report_str, "init last_time_on: %s", time_str);
    report(report_str, 3);

    time = sc::to_time_t(last_time_off);
    strftime(time_str, 64, "%c", std::localtime(&time));
    sprintf(report_str, "init last_time_off: %s", time_str);
    report(report_str, 3);

    report("constructor done", 3);
}
