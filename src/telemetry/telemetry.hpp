
#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#include "../module/module.hpp"
#include <chrono>

namespace location {
    const int Unknown = 1;
    const int Home = 2;
    const int NotHome = 3;
    void loc2str(char* loc_str, int loc);
};

template <class T> class telemetry_value {
    using sc = std::chrono::system_clock;
    using dur = std::chrono::duration<long long, std::ratio<1,1>>;
    using tp = std::chrono::time_point<sc, dur>;
    private:
        T value;
        tp update_time;
    public:
        void set_value(T value) {
            update_time = std::chrono::ceil<dur>(sc::now());
            this->value = value;
        }
        T get_value() {
            return value;
        }
        tp get_update_time() {
            return update_time;
        }
        bool merge(telemetry_value<T>* other) {
            bool change = false;
            if (other->update_time < update_time) {
                *other = *this;
            }
            else {
                change = value != other->value;
                *this = *other;
            }
            return change;
        }
};

class telemetry_values {
    public:
        telemetry_value<int> dad_iphone_home;
        telemetry_value<int> dad_ipad_home;
        telemetry_value<int> mom_iphone_home;
        telemetry_value<int> mom_ipad_home;
        bool merge(telemetry_values* other) {
            bool change = false;
            change = change || dad_iphone_home.merge(&other->dad_iphone_home  );
            change = change ||   dad_ipad_home.merge(&other->dad_ipad_home    );
            change = change || mom_iphone_home.merge(&other->mom_iphone_home);
            change = change ||   mom_ipad_home.merge(&other->mom_ipad_home  );
            return change;
        }
};

class telemetry : public module {
    private:
        telemetry_values* shared_mem = nullptr;
        char name[256];
        int sock;
        bool notify;
        std::mutex mtx;
    protected:
        void get_name(char* name);
        void sync(bool last = false);
    public:
        void sync_values(telemetry_values* t, bool report_only = false);
        telemetry(const char* device, bool notify = true);
};

#endif
