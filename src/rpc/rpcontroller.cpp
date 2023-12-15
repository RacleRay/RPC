#include "rpc/rpccontroller.h"


namespace rayrpc {
    
// Client-side methods ---------------------------------------------
// These calls may be made from the client side only.  Their results
// are undefined on the server side (may crash).

// Resets the RpcController to its initial state so that it may be reused in
// a new call.  Must not be called while an RPC is in progress.
void RpcController::Reset() {
    m_err_code = 0;
    m_err_info = "";
    m_req_id = "";
    m_is_failed = false;
    m_is_canceled = false;
    m_local_addr = nullptr;
    m_peer_addr = nullptr;
    m_timeout = 1000;
}

// After a call has finished, returns true if the call failed.  The possible
// reasons for failure depend on the RPC implementation.  Failed() must not
// be called before a call has finished.  If Failed() returns true, the
// contents of the response message are undefined.       
bool RpcController::Failed() const {                     
    return m_is_failed;                                  
}                                                        
                                                         
// If Failed() is true, returns a human-readable description of the error.
std::string RpcController::ErrorText() const {           
    return m_err_info;                                   
}                                                        
                                                         
// Advises the RPC system that the caller desires that the RPC call be
// canceled.  The RPC system may cancel it immediately, may wait awhile and
// then cancel it, or may not even cancel the call at all.  If the call is
// canceled, the "done" callback will still be called and the RpcController
// will indicate that the call failed at that time.
void RpcController::StartCancel() {
    m_is_canceled = true;          
}

// Server-side methods ---------------------------------------------
// These calls may be made from the server side only.  Their results
// are undefined on the client side (may crash).

// Causes Failed() to return true on the client side.  "reason" will be
// incorporated into the message returned by ErrorText().  If you find
// you need to return machine-readable information about failures, you
// should incorporate it into your response protocol buffer and should
// NOT call SetFailed().
void RpcController::SetFailed(const std::string &reason) {
    m_err_info = reason;
}

// If true, indicates that the client canceled the RPC, so the server may
// as well give up on replying to it.  The server should still call the
// final "done" callback.
bool RpcController::IsCanceled() const {
    return m_is_canceled;
}

// Asks that the given callback be called when the RPC is canceled.  The
// callback will always be called exactly once.  If the RPC completes without
// being canceled, the callback will be called after completion.  If the RPC
// has already been canceled when NotifyOnCancel() is called, the callback
// will be called immediately.
//
// NotifyOnCancel() must be called no more than once per request.
void RpcController::NotifyOnCancel(google::protobuf::Closure *callback) {

}


void RpcController::SetError(int32_t error_code, std::string&& error_info) {
    m_err_code = error_code;
    m_err_info = std::move(error_info);
}

int32_t RpcController::GetErrorCode() const {
    return m_err_code;
}

std::string RpcController::GetErrorInfo() const {
    return m_err_info;
}

void RpcController::SetRequestId(const std::string& req_id) {
    m_req_id = req_id;
}

std::string RpcController::GetRequestId() const {
    return m_req_id;
}

void RpcController::SetLocalAddr(NetAddr::s_ptr addr) {
    m_local_addr = std::move(addr);
}

void RpcController::SetPeerAddr(NetAddr::s_ptr addr) {
    m_peer_addr = std::move(addr);
}

NetAddr::s_ptr RpcController::GetLocalAddr() const {
    return m_local_addr;
}

NetAddr::s_ptr RpcController::GetPeerAddr() const {
    return m_peer_addr;
}

void RpcController::SetTimeout(int timeout) {
    m_timeout = timeout;  // ms
}

int RpcController::GetTimeout() const {
    return m_timeout;
}

void RpcController::SetReqId(const std::string& req_id) {
    m_req_id = req_id;
}

std::string RpcController::GetReqId() {
    return m_req_id;
}

}  // namespace rayrpc