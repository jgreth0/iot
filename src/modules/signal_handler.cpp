
#include "signal_handler.hpp"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
std::map<int,signal_handler*> signal_handler::handlers;
std::mutex signal_handler::mtx;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void signal_handler::sync(bool last) {
    if (!last) notify_listeners();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
signal_handler::signal_handler(int signum) :
        module { false } {
    this->signum = signum;
    char name_full[64];
    snprintf(name_full, 64, "SIGNAL_HANDLER (%d)", signum);
    set_name(name_full);
    std::unique_lock<std::mutex> lck(mtx);
    if (handlers.count(signum)) {
        this->signum = -1;
    } else handlers[signum] = this;
    lck.unlock();
    if (this->signum == -1)
        report("ERROR: multiple handlers with the same signum", 0);
    report("constructor done", 3);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
signal_handler::~signal_handler() {
    std::unique_lock<std::mutex> lck(mtx);
    handlers.erase(signum);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void signal_handler::handler(int signum) {
    std::unique_lock<std::mutex> lck(mtx);
    if (handlers.count(signum))
        handlers[signum]->sync_now();
}
