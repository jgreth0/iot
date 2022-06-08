
#include "presence.hpp"
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

std::mutex mtx;
std::condition_variable cv;
volatile bool done = false;

// Terminate the "loop()" thread on interrupt.
void signalHandler(int signum) {
    std::unique_lock<std::mutex> lck(mtx);
    done = true;
    cv.notify_all();
}

void mgmt() {
    std::unique_lock<std::mutex> lck(mtx);
    // TODO: Allow address by name w/ dns lookup.
    // https://stackoverflow.com/questions/2151854/c-resolve-a-host-ip-address-from-a-url
    char addr[64], name[64];
    strcpy(name, "AIR_FILTER");
    strcpy(addr, "10.72.0.2");
    presence p = presence(name, addr);
    while (!done) cv.wait(lck);
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, signalHandler);
    signal(SIGINT , signalHandler);

    std::thread thread = std::thread(mgmt);
    thread.join();

    return 0;
}
