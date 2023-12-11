#include <arpa/inet.h>
#include <cstring>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

namespace {

int g_pid = 0;

thread_local int t_thread_id = 0;

} // namespace

namespace rayrpc::details {
pid_t get_pid() {
    if (g_pid != 0) { return g_pid; }
    return getpid();
}

pid_t get_thread_id() {
    if (t_thread_id != 0) { return t_thread_id; }
    return syscall(SYS_gettid);
}

// return 'ms' time
int64_t get_now_time() {
    timeval val;
    gettimeofday(&val, nullptr);
    return val.tv_sec * 1000 + val.tv_usec / 1000;
}


int32_t get_int32_from_netbytes(const void* bytes) {
    int32_t val;
    memcpy(&val, bytes, sizeof(val));
    return ntohl(val);
}

} // namespace rayrpc::details