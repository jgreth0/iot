
#include "presence_wrap.hpp"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_wrap::sync(bool last) {
    if (last) {
        update_last();
        return;
    }

    for (int i = 0; i < module_count; i++) {
        if (modules[i].present()) {
            update_present(now_floor());
            return;
        }
    }

    update_not_present();
}

presence_wrap::presence_wrap(char* name, presence* modules, int count) {
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE WRAP [ %s ]", name);
    set_name(name_full);
    // an extra 60 seconds is counted after the child modules all report
    // not_present.
    this->time_limit = duration(60);

    last_time_present = scan_report("DEVICE_PRESENT");
    last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

    report("constructor done", 3);
    module_count = count;
    this->modules = modules;
}

presence_wrap::presence_wrap(const char* name, presence* modules, int count) {
    char name_full[64];
    snprintf(name_full, 64, "PRESENCE WRAP [ %s ]", name);
    set_name(name_full);
    // an extra 60 seconds is counted after the child modules all report
    // not_present.
    this->time_limit = duration(60);

    last_time_present = scan_report("DEVICE_PRESENT");
    last_time_not_present = scan_report("DEVICE_NOT_PRESENT");

    report("constructor done", 3);
    module_count = count;
    this->modules = modules;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_wrap::enable() {
    report("enable()", 4);
    module::enable();
    for (int i = 0; i < module_count; i++)
        modules[i].listen(this);
    report("enable() done", 4);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void presence_wrap::disable() {
    report("disable()", 4);
    for (int i = 0; i < module_count; i++)
        modules[i].unlisten(this);
    module::disable();
    report("disable() done", 4);
}
