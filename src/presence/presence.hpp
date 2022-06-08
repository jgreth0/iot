
#ifndef _PRESENCE_H_
#define _PRESENCE_H_

#include "../module/module.hpp"

class presence : public module {
    private:
        char name[64], addr[64];
        time_point last_time;
        duration time_limit;
        std::mutex mtx;
        bool presence_reported = false;
    protected:
        void get_name(char* name);
        void sync(bool last = false);
    public:
        bool present();
        presence(char* name, char* addr, int time_limit = 60);
};

#endif
