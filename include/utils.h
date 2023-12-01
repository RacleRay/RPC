#pragma once 

#include <sys/types.h>
#include <unistd.h>

namespace rayrpc::details {

pid_t get_pid();

pid_t get_thread_id();

int64_t get_now_time(); // ms

}  // namespace rayrpc::details