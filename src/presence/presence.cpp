
#include "presence.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>

void presence::sync(bool last) {
    if (last) {
        char report_str[256];
        report("DEVICE_PRESENCE_UNKNOWN");
        return;
    }
    int port = 62078;
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

    if (pfd.revents) {
        if (!presence_reported) {
            report("DEVICE_PRESENT");
        }
        presence_reported = true;
        std::unique_lock<std::mutex> lck(mtx);
        last_time = now();
    } else if (!present()) {
        if (presence_reported) {
            report("DEVICE_NOT_PRESENT");
        }
        presence_reported = false;
    }
}

bool presence::present() {
    std::unique_lock<std::mutex> lck(mtx);
    return (now() - last_time) < time_limit;
}

void presence::get_name(char* name) {
    strcpy(name, this->name);
}

presence::presence(char* name, char* addr, int time_limit) :
        module { true, 15 } {
    strcpy(this->addr, addr);
    strcpy(this->name, "PRESENCE [");
    strcat(this->name, name);
    strcat(this->name, " @ ");
    strcat(this->name, addr);
    strcat(this->name, "]");
    this->time_limit = duration(time_limit);
}
