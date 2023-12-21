#include "rpc/rpcclosure.h"
#include "rpc/rpcinterface.h"


namespace rayrpc {
    
void RpcInterface::reply() {
    if (m_closure) {
        m_closure->Run();
    }
}

}  // namespace rayrpc