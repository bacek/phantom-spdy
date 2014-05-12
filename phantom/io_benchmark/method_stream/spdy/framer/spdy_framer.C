
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>

#include <string.h>  // memset

namespace phantom { namespace io_benchmark {

MODULE(io_benchmark_method_stream_spdy_framer);


namespace {
ssize_t do_send(spdylay_session* session,
            const uint8_t* data, size_t length, int flags,
            void *user_data) {
    (void)session;
    (void)flags;
    log_debug("SPDY: framer sending data %ld", length);
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    string_t::ctor_t buf(length);
    for (size_t i = 0; i < length; ++i)
        buf(*data++);
    string_t s = buf;
    self->send_buffer.append(s);

    return length;
}

void on_ctrl_recv_callback(spdylay_session* session,
                              spdylay_frame_type type,
                              spdylay_frame* frame,
                              void* user_data) {
    (void)session;
    (void)type;
    (void)user_data;
    (void)frame;
    log_debug("SPDY: Got CONTROL frame type %d", type);
}

void on_data_recv_callback(spdylay_session* session,
                           uint8_t flags,
                           int32_t stream_id,
                           int32_t length,
                           void* user_data) {
    (void)session;
    (void)flags;
    (void)user_data;
    log_debug("SPDY: Got DATA frame for stream %d length %d flags %d",
            stream_id,
            length,
            flags);
}

}

spdy_framer_t::spdy_framer_t(const ssl_ctx_t& c) : spdy_version(spdy3_1), ctx(c), session(nullptr) {}

spdy_framer_t::~spdy_framer_t() {
    log_debug("SPDY: destructing framer %lx", this);
    spdylay_session_del(session);
}

bool spdy_framer_t::start() {
    log_debug("SPDY: starting framer %lx", this);

    //log_debug("SPDY: proto %s", ctx.negotiated_proto);

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.send_callback = &do_send;
    callbacks.on_ctrl_recv_callback = &on_ctrl_recv_callback;
    callbacks.on_data_recv_callback = &on_data_recv_callback;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session, spdy_version, &callbacks, this);
    return rv == 0;
}


bool spdy_framer_t::receive_data(in_t::ptr_t& in) {
    // recv_buffer = in;
    if (!in)
        return false;
    str_t s = in.__chunk();
    log_debug("SPDY: receive_data %ld", s.size());
    while (s.size() > 0) {
        int processed = spdylay_session_mem_recv(
            session, reinterpret_cast<const uint8_t*>(s.ptr()), s.size());
        log_debug("SPDY: processed data chunk size %d", processed);
        if (processed < 0)
            return false;
        s = str_t(s.ptr() + processed, s.size() - processed);
    }
    return true;
}

bool spdy_framer_t::send_data(in_segment_t &out) {
    spdylay_session_send(session);
    out = send_buffer;
    send_buffer.clear();
    log_debug("SPDY: send_data %lu", out.size());
    return true;
}

}}  // namespace phantom::io_benchmark
