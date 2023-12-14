#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "rpc/reqid.h"

namespace  {
    
int g_req_id_length = 20;
int g_random_fd = -1;

thread_local std::string t_req_id_num;
thread_local std::string t_max_req_id_num;

}  // namespace 

namespace rayrpc {
    
std::string gen_request_id() {
    if (t_req_id_num.empty() || t_req_id_num == t_max_req_id_num) {
        if (g_random_fd < 0) {
            g_random_fd = open("/dev/urandom", O_RDONLY);
        }

        std::string tmp(g_req_id_length, 0);
        if ((read(g_random_fd, tmp.data(), g_req_id_length) != g_req_id_length)) {
            ERRLOG("gen_request_id read from /dev/urandom failed");
            return "";
        }

        for (auto& c : tmp) {
            auto x = c % 10;
            c = (char)('0' + x);
            t_max_req_id_num += "9";
        }

        t_req_id_num = tmp;
    } else {
        int will_not_carry = (int)t_req_id_num.length() - 1;
        while (t_req_id_num[will_not_carry] == '9' && will_not_carry >= 0) {
            will_not_carry--;
        }
        if (will_not_carry >= 0) {
            t_req_id_num[will_not_carry]++;
            for (size_t is_nine = will_not_carry + 1; is_nine < t_req_id_num.length(); is_nine++) {
                t_req_id_num[is_nine] = '0';
            }
        }
    }
    return t_req_id_num;
}

}  // namespace rayrpc