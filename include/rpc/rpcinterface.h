#include <google/protobuf/message.h>

#include "rpcclosure.h"
#include "rpccontroller.h"


namespace rayrpc {

#define DELETE_RESOURCE(RESO)     \
    if ((RESO) != NULL) {         \
        delete (RESO);            \
        (RESO) = NULL;            \
    }                           \

/*
 * Rpc Interface Base Class
 * All interface should extend this abstract class
 * 
 * Will be invoked at RpcClosure.Run(). 
*/
class RpcInterface : public std::enable_shared_from_this<RpcInterface> {
  public:
    // RpcInterface(RpcClosure *done, RpcController *controller) : m_done(done), m_controller(controller) {};

    RpcInterface(const google::protobuf::Message* req, google::protobuf::Message* rsp, RpcClosure *done, RpcController *controller)
        : m_request(req), m_response(rsp), m_done(done), m_controller(controller) 
    {
        INFOLOG("RpcInterface created.");
    };

    virtual ~RpcInterface() {
        INFOLOG("RpcInterface destroyed.");
        reply();
        destroy();
    }

    // core business deal method
    virtual void run() = 0;

    // set error code and error into to response message
    virtual void setError(int64_t code, const std::string &err_info) = 0;

    // reply to client
    // you should call is when you want to response back. 
    // invoke done function, then a rpc call ends.
    virtual void reply() {
        if (m_done) {
            m_done->Run();
        }
    }

    // free resourse
    void destroy() {
        DELETE_RESOURCE(m_request);
        DELETE_RESOURCE(m_response);
        DELETE_RESOURCE(m_done);
        DELETE_RESOURCE(m_controller);
    }

    // alloc a closure object which handle by this interface
    std::shared_ptr<RpcClosure> newRpcClosure(std::function<void()>& cb) {
        return std::make_shared<RpcClosure>(shared_from_this(),cb);
    }

  protected:
    RpcClosure *m_done{nullptr};  // callback

    RpcController *m_controller{nullptr};

    const google::protobuf::Message* m_request{nullptr};

    google::protobuf::Message* m_response{nullptr};

}; // class RpcInterface

} // namespace rayrpc