#pragma once 

#include <sys/types.h>
#include <unistd.h>

namespace rayrpc::details {

pid_t get_pid();

pid_t get_thread_id();

int64_t get_now_time(); // ms

int32_t get_int32_from_netbytes(const void* bytes);

}  // namespace rayrpc::details