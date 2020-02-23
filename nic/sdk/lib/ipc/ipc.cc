//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#include "ipc.hpp"
#include "ipc_internal.hpp"
#include "zmq_ipc.hpp"
#include "subscribers.hpp"

#include "include/sdk/base.hpp"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace sdk {
namespace ipc {

typedef struct req_callback_ {
    request_cb cb;
    const void *ctx;
} req_callback_t;

typedef struct rsp_callback_ {
    response_cb cb;
    const void *ctx;
} rsp_callback_t;

typedef struct sub_callback_ {
    subscription_cb cb;
    const void *ctx;
} sub_callback_t;

class ipc_service {
public:
    ~ipc_service();
    ipc_service();
    ipc_service(uint32_t client_id);
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie) = 0;
    void respond(ipc_msg_ptr msg, const void *data, size_t data_length);
    void broadcast(uint32_t msg_code, const void *data, size_t data_length);
    void reg_request_handler(uint32_t msg_code, request_cb callback,
                             const void *ctx);
    void reg_response_handler(uint32_t msg_code, response_cb callback,
                              const void *ctx);
    void subscribe(uint32_t msg_code, subscription_cb callback,
                   const void *ctx);
    void receive(void);
    void server_receive(void);
    void eventfd_receive(void);
    void set_drip_feeding(bool drip_feed);
protected: 
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) = 0;
    uint32_t get_id_(void);
    void set_server_(zmq_ipc_server_ptr ipc_server);
    void handle_response_(ipc_msg_ptr msg, response_oneshot_cb cb,
                          const void *cookie);
    zmq_ipc_client_ptr get_client_(uint32_t recipient);
    bool should_serialize_(void);
    void serialize_(ipc_msg_ptr msg);
    void deserialize_(void);
    void deliver_(ipc_msg_ptr msg);
    int get_eventfd_(void);
private:
    uint32_t id_;
    zmq_ipc_server_ptr ipc_server_;
    zmq_ipc_client_ptr ipc_clients_[IPC_MAX_ID + 1];
    std::map<uint32_t, req_callback_t> req_cbs_;
    std::map<uint32_t, rsp_callback_t> rsp_cbs_;
    std::map<uint32_t, sub_callback_t> sub_cbs_;
    // the fields below are used for serialized delivery
    bool serializing_enabled_;
    bool message_in_flight_;
    // hold queues are used to serialize messages
    std::queue<ipc_msg_ptr> hold_queue_;
    // deliver queue is used to actually deliver held messages to
    // the client at the next tick
    std::queue<ipc_msg_ptr> delivery_queue_;
    // eventfd is used to notify the client that things are ready in
    // delivery_queue_
    int eventfd_;
};
typedef std::shared_ptr<ipc_service> ipc_service_ptr;

class ipc_service_sync : public ipc_service {
public:
    ipc_service_sync();
    ipc_service_sync(uint32_t client_id);
    ipc_service_sync(uint32_t client_id, fd_watch_cb fd_watch_cb,
                     const void *fd_watch_cb_ctx);
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie) override;
protected:
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) override;
};
typedef std::shared_ptr<ipc_service_sync> ipc_service_sync_ptr;

class ipc_service_async : public ipc_service {
public:
    ipc_service_async(uint32_t client_id, fd_watch_cb fd_watch_cb,
                      const void *fd_watch_cb_ctx);
    ipc_service_async(uint32_t client_id, fd_watch_ms_cb fd_watch_ms_cb);
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie) override;
    void client_receive(uint32_t recipient);
protected:
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) override;
private:
    ipc_service_async(uint32_t client_id, fd_watch_ms_cb fd_watch_ms_cb,
                      fd_watch_cb fd_watch_cb, const void *fd_watch_cb_ctx);
    fd_watch_ms_cb fd_watch_ms_cb_;
    fd_watch_cb fd_watch_cb_;
    const void *fd_watch_cb_ctx_;
};
typedef std::shared_ptr<ipc_service_async> ipc_service_async_ptr;

thread_local ipc_service_ptr t_ipc_service = nullptr;

static void
server_receive (int fd, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->server_receive();
}

static void
eventfd_receive (int fd, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->eventfd_receive();
}

static void
server_receive_ms (int fd, int, void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->server_receive();
}

static void
client_receive (int fd, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    uint32_t recipient = (uint64_t)ctx;
    std::dynamic_pointer_cast<ipc_service_async>(t_ipc_service)->client_receive(
        recipient);
}

static void
client_receive_ms (int fd, int, void *ctx)
{
    assert(t_ipc_service != nullptr);
    uint32_t recipient = (uint64_t)ctx;
    std::dynamic_pointer_cast<ipc_service_async>(t_ipc_service)->client_receive(
        recipient);
}

ipc_service::~ipc_service() {
    if (this->eventfd_ != -1) {
        close(this->eventfd_);
    }
}

ipc_service::ipc_service() : ipc_service(IPC_MAX_ID + 1) {};

ipc_service::ipc_service(uint32_t id) {
    this->id_ = id;
    this->ipc_server_ = nullptr;
    this->eventfd_ = -1;
    this->serializing_enabled_ = false;
    
    for (int i = 0; i < IPC_MAX_ID + 1; i++) {
        this->ipc_clients_[i] = nullptr;
    }
}

uint32_t
ipc_service::get_id_(void) {
    return this->id_;
}

void
ipc_service::set_server_(zmq_ipc_server_ptr server) {
    assert(this->ipc_server_ == nullptr);
    this->ipc_server_ = server;
    
    this->eventfd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    assert(this->eventfd_ != -1);
}

zmq_ipc_client_ptr
ipc_service::get_client_(uint32_t recipient) {
    zmq_ipc_client_ptr client;

    client = this->ipc_clients_[recipient];
    if (client == nullptr) {
        client = this->new_client_(recipient);
        
        this->ipc_clients_[recipient] = client;
    }
    return client;
}

void
ipc_service::respond(ipc_msg_ptr msg, const void *data, size_t data_length) {
    assert(msg != nullptr);
    this->ipc_server_->reply(msg, data, data_length);
    this->message_in_flight_ = false;
    this->deserialize_();
}

void
ipc_service::handle_response_(ipc_msg_ptr msg, response_oneshot_cb cb,
                              const void *cookie) {
    if (cb) {
        cb(msg, cookie);
    } else {
        assert(this->rsp_cbs_.count(msg->code()) > 0);
        rsp_callback_t rsp_cb = this->rsp_cbs_[msg->code()];
        if (rsp_cb.cb != NULL) {
            rsp_cb.cb(msg, cookie, rsp_cb.ctx);
        }
    }
}

ipc_service_sync::ipc_service_sync() {
}

ipc_service_sync::ipc_service_sync(uint32_t client_id)
    : ipc_service(client_id) {
}

ipc_service_sync::ipc_service_sync(uint32_t client_id, fd_watch_cb fd_watch_cb,
                                    const void *fd_watch_cb_ctx)
    : ipc_service(client_id) {
 
    zmq_ipc_server_ptr server = std::make_shared<zmq_ipc_server>(
        this->get_id_());

    this->set_server_(server);
    fd_watch_cb(server->fd(), sdk::ipc::server_receive, NULL, fd_watch_cb_ctx);
}

void
ipc_service_sync::request(uint32_t recipient, uint32_t msg_code,
                          const void *data, size_t data_length,
                          response_oneshot_cb cb, const void *cookie) {

    zmq_ipc_client_sync_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_sync>(
            this->get_client_(recipient));

    ipc_msg_ptr msg = client->send_recv(msg_code, data, data_length);

    this->handle_response_(msg, cb, cookie);
}

zmq_ipc_client_ptr
ipc_service_sync::new_client_(uint32_t recipient) {
    return std::make_shared<zmq_ipc_client_sync>(recipient);
}

ipc_service_async::ipc_service_async(uint32_t client_id,
                                     fd_watch_ms_cb fd_watch_ms_cb,
                                     fd_watch_cb fd_watch_cb,
                                     const void *fd_watch_cb_ctx)
    : ipc_service(client_id) {

    this->fd_watch_ms_cb_ = fd_watch_ms_cb;
    this->fd_watch_cb_ = fd_watch_cb;
    this->fd_watch_cb_ctx_ = fd_watch_cb_ctx;

    zmq_ipc_server_ptr server = std::make_shared<zmq_ipc_server>(
        this->get_id_());

    this->set_server_(server);
    if (this->fd_watch_ms_cb_) {
        this->fd_watch_ms_cb_(server->fd(), sdk::ipc::server_receive_ms, NULL);
        // todo: enable serialized messages for ms thread maybe??
    } else {
        this->fd_watch_cb_(server->fd(), sdk::ipc::server_receive, NULL,
                           this->fd_watch_cb_ctx_);
        this->fd_watch_cb_(this->get_eventfd_(), sdk::ipc::eventfd_receive, NULL,
                           this->fd_watch_cb_ctx_);
    }    
}

ipc_service_async::ipc_service_async(uint32_t client_id,
                                     fd_watch_cb fd_watch_cb,
                                     const void *fd_watch_cb_ctx)
    : ipc_service_async(client_id, NULL, fd_watch_cb, fd_watch_cb_ctx) {
}

ipc_service_async::ipc_service_async(uint32_t client_id,
                                     fd_watch_ms_cb fd_watch_ms_cb)
    : ipc_service_async(client_id, fd_watch_ms_cb, NULL, NULL) {
}

zmq_ipc_client_ptr
ipc_service_async::new_client_(uint32_t recipient) {
    zmq_ipc_client_async_ptr client =
        std::make_shared<zmq_ipc_client_async>(this->get_id_(), recipient);

    if (this->fd_watch_ms_cb_) {
        this->fd_watch_ms_cb_(client->fd(), sdk::ipc::client_receive_ms,
                              (void *)(uint64_t)recipient);
    } else {
        this->fd_watch_cb_(client->fd(), sdk::ipc::client_receive,
                           (void *)(uint64_t)recipient, this->fd_watch_cb_ctx_);
    }
        
    // If we don't do this we don't get any events coming from ZMQ
    // It doesn't play very well with libevent
    client->recv();

    return client;
}

void
ipc_service_async::request(uint32_t recipient, uint32_t msg_code,
                           const void *data, size_t data_length,
                           response_oneshot_cb cb, const void *cookie) {

    zmq_ipc_client_async_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_async>(
            this->get_client_(recipient));

    client->send(msg_code, data, data_length, cb, cookie);
}

void
ipc_service_async::client_receive(uint32_t sender) {
    assert(sender < IPC_MAX_ID + 1);

    zmq_ipc_client_async_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_async>(
            this->get_client_(sender));

    while (client->is_event_pending()) {
        zmq_ipc_user_msg_ptr msg = client->recv();
        if (msg == nullptr) {
            return;
        }

        this->handle_response_(msg, msg->response_cb(), msg->cookie());
    }
}


bool
ipc_service::should_serialize_(void) {
    return (this->serializing_enabled_ && this->message_in_flight_);
}

void
ipc_service::serialize_(ipc_msg_ptr msg) {
    SDK_TRACE_DEBUG("Serializing message with msg_code: %u", msg->code());
    this->hold_queue_.push(msg);
}

void
ipc_service::deserialize_(void) {
    uint64_t buffer = 1;
    
    if (this->hold_queue_.size() == 0) {
        return;
    }

    SDK_TRACE_DEBUG("Messages waiting: %i", this->hold_queue_.size());
    
    // Notify the client we have messages for delivery
    write(this->eventfd_, &buffer, sizeof(buffer));
}

void
ipc_service::deliver_(ipc_msg_ptr msg) {
    // We received an IPC message but don't have a handler
    assert(this->req_cbs_.count(msg->code()) > 0);
                
    req_callback_t req_cb = this->req_cbs_[msg->code()];
                
    if (req_cb.cb != NULL) {
        this->message_in_flight_ = true;
        req_cb.cb(msg, req_cb.ctx);
    }
}

int
ipc_service::get_eventfd_(void) {
    return this->eventfd_;
}

void
ipc_service::server_receive(void) {
    while (true) {
        ipc::ipc_msg_ptr msg = this->ipc_server_->recv();
        if (msg == nullptr) {
            return;
        }
        if (msg->type() == sdk::ipc::DIRECT) {
            if (this->should_serialize_()) {
                this->serialize_(msg);
            } else {
                this->deliver_(msg);
            }
        } else if (msg->type() == sdk::ipc::BROADCAST) {
            // We shouldn't be receiving this if we don't have a handler
            assert(this->sub_cbs_.count(msg->code()) > 0);

            sub_callback_t sub_cb = this->sub_cbs_[msg->code()];;

            if (sub_cb.cb != NULL) {
                sub_cb.cb(msg, sub_cb.ctx);
            }
        } else {
            assert(0);
        }
    }
}

void
ipc_service::eventfd_receive(void) {
    uint64_t buffer;

    // Clear the eventfd flag
    read(this->eventfd_, &buffer, sizeof(buffer));;
    
    while(!this->hold_queue_.empty()) {
        ipc_msg_ptr msg = this->hold_queue_.front();
        this->hold_queue_.pop();
        SDK_TRACE_DEBUG("Delivering postponed message for msg_code: %u",
                        msg->code());
        this->deliver_(msg);
    }
}

void
ipc_service::broadcast(uint32_t msg_code, const void *data,
                       size_t data_length) {
    std::vector<uint32_t> recipients = subscribers::instance()->get(msg_code);

    if (recipients.size() == 0) {
        SDK_TRACE_DEBUG("No subscribers for message: msg_code: %u", msg_code);
    }
    for (uint32_t recipient : recipients) {
        this->get_client_(recipient)->broadcast(msg_code, data, data_length);
    }
}

void
ipc_service::reg_request_handler(uint32_t msg_code, request_cb callback,
                                 const void *ctx) {
    assert(this->req_cbs_.count(msg_code) == 0);
    this->req_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
}

void
ipc_service::reg_response_handler(uint32_t msg_code, response_cb callback,
                                  const void *ctx) {
    assert(this->rsp_cbs_.count(msg_code) == 0);
    this->rsp_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
}

void
ipc_service::subscribe(uint32_t msg_code, subscription_cb callback,
                       const void *ctx) {
    assert(this->sub_cbs_.count(msg_code) == 0);
    this->sub_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
    this->ipc_server_->subscribe(msg_code);
}

void
ipc_service::set_drip_feeding(bool enabled) {
    this->serializing_enabled_ = enabled;
}

// We use this as for non asynchronous thread creating a client
// upfront is not required for request or broadcast.
// We create the first time an API gets called
ipc_service_ptr
service (void)
{
    if (t_ipc_service == nullptr) {
        t_ipc_service = std::make_shared<ipc_service_sync>();
    }
    return t_ipc_service;
}

void
ipc_init_async (uint32_t client_id, fd_watch_cb fd_watch_cb,
                const void *fd_watch_cb_ctx)
{
    assert(t_ipc_service == nullptr);
    t_ipc_service = std::make_shared<ipc_service_async>(
        client_id, fd_watch_cb, fd_watch_cb_ctx);
}

void
ipc_init_metaswitch (uint32_t client_id, fd_watch_ms_cb fd_watch_ms_cb)
{
    t_ipc_service = nullptr;
    t_ipc_service = std::make_shared<ipc_service_async>(client_id,
                                                        fd_watch_ms_cb);
}

void
ipc_init_sync (uint32_t client_id)
{
    assert(t_ipc_service == nullptr);
    t_ipc_service = std::make_shared<ipc_service_sync>(client_id);
}

void
ipc_init_sync (uint32_t client_id, fd_watch_cb fd_watch_cb,
               const void *fd_watch_cb_ctx)
{
    assert(t_ipc_service == nullptr);
    t_ipc_service = std::make_shared<ipc_service_sync>(
        client_id, fd_watch_cb, fd_watch_cb_ctx);
}


void
request (uint32_t recipient, uint32_t msg_code, const void *data,
         size_t data_length, const void *cookie)
{
    service()->request(recipient, msg_code, data, data_length, NULL, cookie);
}

void
request (uint32_t recipient, uint32_t msg_code, const void *data,
         size_t data_length, response_oneshot_cb cb, const void *cookie)
{
    service()->request(recipient, msg_code, data, data_length, cb, cookie);
}

void
broadcast (uint32_t msg_code, const void *data, size_t data_length)
{
    service()->broadcast(msg_code, data, data_length);
}

// We need to call init or init_async before using this method
void
reg_request_handler (uint32_t msg_code, request_cb callback, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->reg_request_handler(msg_code, callback, ctx);
}

void
reg_response_handler (uint32_t msg_code, response_cb callback, const void *ctx)
{
    service()->reg_response_handler(msg_code, callback, ctx);
}

// We need to call init or init_async before using this method
void
subscribe (uint32_t msg_code, subscription_cb callback, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->subscribe(msg_code, callback, ctx);
}

void
respond (ipc_msg_ptr msg, const void *data, size_t data_length)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->respond(msg, data, data_length);
}

void
set_drip_feeding (bool enabled)
{
    service()->set_drip_feeding(enabled);
}

} // namespace ipc
} // namespace sdk
