#include "log.h"

#include "rpc/rpcclosure.h"
#include "rpc/rpccontroller.h"
#include "rpc/rpcinterface.h"


namespace rayrpc {

#define DELETE_RESOURCE(RESO)     \
    if ((RESO) != NULL) {         \
        delete (RESO);            \
        (RESO) = NULL;            \
    }                             \


void RpcInterface::reply() {
    if (m_done) {
        m_done->Run();
    }
}


void RpcInterface::destroy() {
    // 参考，统一释放资源
    // DELETE_RESOURCE(m_request);
    // DELETE_RESOURCE(m_response);
    // DELETE_RESOURCE(m_done);
    // DELETE_RESOURCE(m_controller);
}

}  // namespace rayrpc