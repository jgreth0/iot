
#include "shmem.hpp"
#include <cstring>
#include <stdio.h>
#include <csignal>
#include <mutex>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
// Open the file as shared memory
////////////////////////////////////////////////////////////////////////////////
shmem::shmem(const char* device, int size, signal_handler* sigio_handler) :
        module { false } {
    char name[64];
    snprintf(name, 64, "SHMEM [ %s ]", device);
    set_name(name);
    report("constructor called", 5);
    if (shared_mem != nullptr) return;
    sock = open(device, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    shared_mem = mmap(NULL, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, sock, 0);
    target_value = calloc(size, 1);
    result_value = calloc(size, 1);
    this->size = size;
    this->sigio_handler = sigio_handler;
    if (sigio_handler) fcntl(sock, F_SETOWN, getpid());
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void shmem::sync(bool last) {
    std::unique_lock<std::mutex> lck(mtx);
    uint8_t target_value[size], result_value[size];
    bool target_valid = this->target_valid;
    memcpy(target_value, this->target_value, size);
    lck.unlock();

    // Lock the file to avoid races between processes.
    struct flock flock = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = size,
        .l_pid = 0};
    if (target_valid) flock.l_type = F_WRLCK;
    fcntl(sock, F_SETLKW, &flock);

    if (target_valid) memcpy(shared_mem, &target_value, size);
    memcpy(result_value, shared_mem, size);

    // Unlock the file. All accesses to shared_mem are complete.
    flock.l_type = F_UNLCK;
    fcntl(sock, F_SETLK, flock);

    if (target_valid && !sigio_handler)
        kill(fcntl(sock, F_GETOWN), SIGIO);

    lck.lock();
    if (0 == memcmp(this->result_value, result_value, size)) {
        last_change = now_floor();
        notify_listeners();
    }
    memcpy(this->result_value, result_value, size);
    // target==result, further updates are not needed.
    if (0 == memcmp(this->result_value, this->target_value, size))
        this->target_valid = false;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
shmem::time_point shmem::get_result(void* value) {
    std::unique_lock<std::mutex> lck(mtx);
    if (value != nullptr) memcpy(value, result_value, size);
    return last_change;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void shmem::set_target(void* value) {
    std::unique_lock<std::mutex> lck(mtx);
    memcpy(target_value, value, size);
    target_valid = true;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
shmem::~shmem() {
    munmap(shared_mem, size + sizeof(time_point));
    free(target_value);
    free(result_value);
    close(sock);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void shmem::enable() {
    report("enable()", 4);
    module::enable();
    if (sigio_handler) sigio_handler->listen(this);
    report("enable() done", 4);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void shmem::disable() {
    report("disable()", 4);
    if (sigio_handler) sigio_handler->unlisten(this);
    module::disable();
    report("disable() done", 4);
}
