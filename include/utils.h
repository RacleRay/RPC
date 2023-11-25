#pragma once 

#include <sys/types.h>
#include <unistd.h>

namespace rayrpc::details {

pid_t get_pid();

pid_t get_thread_id();

}  // namespace rayrpc::details