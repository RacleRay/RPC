#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <string>

#include "tcp/netaddr.h"


namespace rayrpc {

// An RpcController mediates a single method call.  The primary purpose of
// the controller is to provide a way to manipulate settings specific to the
// RPC implementation and to find out about RPC-level errors.
class RpcController : public google::protobuf::RpcController {
  public:
    RpcController() = default;
    ~RpcController() override = default ;

    // Client-side methods ---------------------------------------------
    // These calls may be made from the client side only.  Their results
    // are undefined on the server side (may crash).

    // Resets the RpcController to its initial state so that it may be reused in
    // a new call.  Must not be called while an RPC is in progress.
    void Reset() override;

    // After a call has finished, returns true if the call failed.  The possible
    // reasons for failure depend on the RPC implementation.  Failed() must not
    // be called before a call has finished.  If Failed() returns true, the
    // contents of the response message are undefined.
    [[nodiscard]] bool Failed() const override;

    // If Failed() is true, returns a human-readable description of the error.
    [[nodiscard]] std::string ErrorText() const override;

    // Advises the RPC system that the caller desires that the RPC call be
    // canceled.  The RPC system may cancel it immediately, may wait awhile and
    // then cancel it, or may not even cancel the call at all.  If the call is
    // canceled, the "done" callback will still be called and the RpcController
    // will indicate that the call failed at that time.
    void StartCancel() override;

    // Server-side methods ---------------------------------------------
    // These calls may be made from the server side only.  Their results
    // are undefined on the client side (may crash).

    // Causes Failed() to return true on the client side.  "reason" will be
    // incorporated into the message returned by ErrorText().  If you find
    // you need to return machine-readable information about failures, you
    // should incorporate it into your response protocol buffer and should
    // NOT call SetFailed().
    void SetFailed(const std::string &reason) override;

    // If true, indicates that the client canceled the RPC, so the server may
    // as well give up on replying to it.  The server should still call the
    // final "done" callback.
    [[nodiscard]] bool IsCanceled() const override;

    // Asks that the given callback be called when the RPC is canceled.  The
    // callback will always be called exactly once.  If the RPC completes without
    // being canceled, the callback will be called after completion.  If the RPC
    // has already been canceled when NotifyOnCancel() is called, the callback
    // will be called immediately.
    //
    // NotifyOnCancel() must be called no more than once per request.
    void NotifyOnCancel(google::protobuf::Closure *callback) override;


    void SetError(int32_t error_code, std::string&& error_info);

    [[nodiscard]] int32_t GetErrorCode() const;

    [[nodiscard]] std::string GetErrorInfo() const;

    void SetRequestId(const std::string& req_id);

    [[nodiscard]] std::string GetRequestId() const;

    void SetLocalAddr(NetAddr::s_ptr addr);

    void SetPeerAddr(NetAddr::s_ptr addr);

    [[nodiscard]] NetAddr::s_ptr GetLocalAddr() const;

    [[nodiscard]] NetAddr::s_ptr GetPeerAddr() const;

    void SetTimeout(int timeout);

    [[nodiscard]] int GetTimeout() const;

    void SetReqId(const std::string& req_id);

    std::string GetReqId();

  private:
    int32_t m_err_code{0};

    bool m_is_failed{false};
    bool m_is_canceled{false};

    std::string m_err_info;
    std::string m_req_id;

    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    int m_timeout{1000};  // ms
}; // class RpcController

} // namespace rayrpc