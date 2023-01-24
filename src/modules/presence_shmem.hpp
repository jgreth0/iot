
#ifndef _PRESENCE_shmem_H_
#define _PRESENCE_shmem_H_

#include "presence.hpp"
#include "shmem.hpp"
#include "signal_handler.hpp"

////////////////////////////////////////////////////////////////////////////////
// this module detects the presence of a device based on some external process.
// When the device is seen, the external process updates a shared memory file.
// This module observes the update and responds accordingly.
////////////////////////////////////////////////////////////////////////////////
class presence_shmem : public presence {
private:
    shmem* sm;

protected:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    void sync(bool last = false);
public:
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    presence_shmem(char* name, char* addr, signal_handler* sh, int time_limit = 300);
    presence_shmem(const char* name, const char* addr, signal_handler* sh, int time_limit = 300);
    ~presence_shmem();
};

#endif
