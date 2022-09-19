
#include "presence_tcp.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_tcp::sync(bool last) {
    if (last) {
        report("DEVICE_PRESENCE_UNKNOWN", 2, true);
        return;
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    int res = poll(&pfd, 1, 100);

    char report_str[256];
    snprintf(report_str, 256, "poll result = (%d); revents = 0x%x", res, pfd.revents);
    report(report_str, 6);

    std::unique_lock<std::mutex> lck(mtx);

    // Refresh last_time_not_present so that it will equal the precise time that
    // the device is first seen again.
    if (!((now_floor() - last_time_present) < time_limit))
        last_time_not_present = now_floor();
    // last_time_present is updated only at the times when we actually see the device.

    if (pfd.revents == POLLOUT) {
        // The device was seen.
        last_time_present = now_floor();
        lck.unlock();
        if (!presence_reported || !last_reported) {
            report("DEVICE_PRESENT", 2, true);
            notify_listeners();
        }
        presence_reported = true;
        last_reported = true;
    } else if (!((now_floor() - last_time_present) < time_limit)) {
        // The device has not been seen for a while.
        last_time_not_present = now_floor();
        lck.unlock();
        if (!presence_reported || last_reported) {
            report("DEVICE_NOT_PRESENT", 2, true);
            notify_listeners();
        }
        presence_reported = true;
        last_reported = false;
    }
    close(sock);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
bool presence_tcp::present() {
    report("present() called", 5);
    std::unique_lock<std::mutex> lck(mtx);
    bool p = (now_floor() - last_time_present) < time_limit;
    lck.unlock();
    report("present() complete", 5);
    return p;
}

////////////////////////////////////////////////////////////////////////////
// When was the device last detected
// If the device was detected within the time limit, returns now_floor()
////////////////////////////////////////////////////////////////////////////
presence_tcp::time_point presence_tcp::get_last_time_present() {
    report("get_last_time_present() called", 5);
    std::unique_lock<std::mutex> lck(mtx);
    bool p = (now_floor() - last_time_present) < time_limit;
    time_point t = last_time_present;
    if (p) t = now_floor();
    lck.unlock();
    report("get_last_time_present() complete", 5);
    return t;
}

////////////////////////////////////////////////////////////////////////////
// When was the device last undetected for at least time_limit seconds?
// If the device was not detected within the time limit, returns now_floor()
////////////////////////////////////////////////////////////////////////////
presence_tcp::time_point presence_tcp::get_last_time_not_present() {
    report("get_last_time_not_present() called", 5);
    std::unique_lock<std::mutex> lck(mtx);
    bool p = (now_floor() - last_time_present) < time_limit;
    time_point t = last_time_not_present;
    if (!p) t = now_floor();
    lck.unlock();
    report("get_last_time_not_present() complete", 5);
    return t;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
presence_tcp::presence_tcp(char* name, char* addr, int port, int time_limit) {
    strncpy(this->addr, addr, 64);
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE TCP [ %s @ %s ]", name, addr);
    set_name(name_full);
    this->time_limit = duration(time_limit);
    last_time_present = now_floor() - this->time_limit;
    last_time_not_present = last_time_not_present;
    this->port = port;
    report("constructor done", 3);
}
