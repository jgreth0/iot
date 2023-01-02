
#include "presence_icmp.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>

#include <stdio.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

uint16_t checksum(void *b, int len) {
    uint16_t *buf = (uint16_t*)b;
    uint32_t sum = 0;

    for (; len > 1; len -= 2) sum += *buf++;
    if ( len == 1 ) sum += *(uint8_t*)buf;
    while (sum > 0xFFFF) sum = (sum >> 16) + (sum & 0xFFFF);
    return (uint16_t)(~sum);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_icmp::sync(bool last) {
    char report_str[256];

    if (last) {
        report("DEVICE_PRESENCE_UNKNOWN", 2, true);
        close(sock);
        return;
    }

    // Wait a random amount, up to 500ms, to avoid bursts.
    usleep(1000 * (rand() % 500));

    // SOCK_RAW may be needed on other systems.
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sock < 0) {
        snprintf(report_str, 256, "Socket Error: %d", errno);
        report(report_str, 5);
    } else {
        report("Socket created", 5);
    }

    struct timeval tv_out = {.tv_usec = 100};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);
    int ttl_val = 8;
    setsockopt(sock, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));

    struct icmphdr pkt = {.type = ICMP_ECHO};
    pkt.un.echo.id = htons(getpid() & 0xFFFF);
    pkt.un.echo.sequence = seq_num++;
    pkt.checksum = checksum(&pkt, sizeof(pkt));

    struct sockaddr_in send_addr = {.sin_family = AF_INET}, recv_addr;
    inet_pton(AF_INET, addr, &send_addr.sin_addr);

    int res = sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));

    snprintf(report_str, 256, "Send result: %d", res);
    report(report_str, 5);

    int addr_len = sizeof(recv_addr);
    res = recvfrom(sock, &pkt, sizeof(pkt), 0,(struct sockaddr*)&recv_addr, (socklen_t *)&addr_len);

    snprintf(report_str, 256, "Receive result: %d, %d", res, pkt.type);
    report(report_str, 5);
    close(sock);
    sock = -1;

    std::unique_lock<std::mutex> lck(mtx);

    // Refresh last_time_not_present so that it will equal the precise time that
    // the device is first seen again.
    if (!((now_floor() - last_time_present) < time_limit))
        last_time_not_present = now_floor();
    // last_time_present is updated only at the times when we actually see the device.

    if (pkt.type == ICMP_ECHOREPLY) {
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
bool presence_icmp::present() {
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
presence_icmp::time_point presence_icmp::get_last_time_present() {
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
presence_icmp::time_point presence_icmp::get_last_time_not_present() {
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
presence_icmp::presence_icmp(char* name, char* addr, int time_limit) {
    strncpy(this->addr, addr, 64);
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE ICMP [ %s @ %s ]", name, addr);
    set_name(name_full);
    this->time_limit = duration(time_limit);
    last_time_present = now_floor() - this->time_limit;
    last_time_not_present = last_time_not_present;

    report("constructor done", 3);
}
