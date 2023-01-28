
#ifndef _AUTOMATION_GROUP_H_
#define _AUTOMATION_GROUP_H_

#include "automation.hpp"
#include <cstring>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class automation_group : public automation {
private:
    std::vector<automation*> automations;
    std::mutex mtx;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(time_point current_time) {
        std::unique_lock<std::mutex> lck(mtx);
        for (int i = 0; i < automations.size(); i++)
            automations[i]->sync(current_time);
    }

public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    automation_group(const char* name) {
        char name_full[64];
        snprintf(name_full, 64, "AUTOMATION_GROUP [ %s ]", name);
        set_name(name_full);
        report("constructor done", 3);
    }

    void add_automation(automation* a) {
        std::unique_lock<std::mutex> lck(mtx);
        automations.push_back(a);
    }
};

#endif
