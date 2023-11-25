#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

namespace {

int g_pid = 0;

thread_local int g_thread_id = 0;

} // namespace

namespace rayrpc::details {
pid_t get_pid() {
    if (g_pid != 0) { return g_pid; }
    return getpid();
}

pid_t get_thread_id() {
    if (g_thread_id != 0) { return g_thread_id; }
    return syscall(SYS_gettid);
}
} // namespace rayrpc::details