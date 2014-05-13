
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/exception.H>

#include <string.h>  // memset

#include "spdy_misc.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

namespace {
ssize_t do_send(spdylay_session* UNUSED(session),
            const uint8_t* data, size_t length, int UNUSED(flags),
            void *user_data) {
    log_debug("SPDY: framer sending data %ld", length);
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    string_t::ctor_t buf(length);
    for (size_t i = 0; i < length; ++i)
        buf(*data++);
    string_t s = buf;
    self->send_buffer.append(s);

    return length;
}

void on_ctrl_recv_callback(spdylay_session* UNUSED(session),
                              spdylay_frame_type type,
                              spdylay_frame* frame,
                              void* user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    log_debug("SPDY: Got CONTROL frame type %d %d %ld",
        type,
        frame->ctrl.hd.flags,
        self->in_flight_requests);

    if (type == SPDYLAY_SYN_REPLY) {
        if (frame->syn_reply.hd.flags & SPDYLAY_CTRL_FLAG_FIN)
            self->in_flight_requests--;

        // Search for :status and extract it
        // Iterate over even headers.
        for (ssize_t i = 0; frame->syn_reply.nv[i]; i +=2 ) {
            if (strncmp(frame->syn_reply.nv[i], ":status", 7) == 0) {
                char* end;
                self->last_res_code = strtoul(frame->syn_reply.nv[i+1], &end, 10);
                if (*end != ' ') {
                    log_debug("Failed to parse status line");
                    self->last_res_code = 500;
                }
                log_debug("SPDY: status '%d'", self->last_res_code);
                break;
            }
        }
    } else if (type == SPDYLAY_GOAWAY) {
        // It's not actually true. But it will work for now
        self->last_res_code = 500;
    }
}

void on_data_recv_callback(spdylay_session* UNUSED(session),
                           uint8_t flags,
                           int32_t stream_id,
                           int32_t length,
                           void* user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);
    log_debug("SPDY: Got DATA frame for stream %d length %d flags %d (%ld)",
            stream_id,
            length,
            flags,
            self->in_flight_requests);
    if (flags & SPDYLAY_DATA_FLAG_FIN)
        self->in_flight_requests--;
}

}

spdy_framer_t::spdy_framer_t(const ssl_ctx_t& c) : spdy_version(spdy3_1), ctx(c), session(nullptr), in_flight_requests(0) {}

spdy_framer_t::~spdy_framer_t() {
    log_debug("SPDY: destructing framer %lx", this);
    spdylay_session_del(session);
}

bool spdy_framer_t::start() {
    log_debug("SPDY: starting framer %lx", this);

    if (string_t::cmp_eq<ident_t>(ctx.negotiated_proto, CSTR("spdy/2")))
        spdy_version = spdy2;
    else if (string_t::cmp_eq<ident_t>(ctx.negotiated_proto, CSTR("spdy/3")))
        spdy_version = spdy3;
    else if (string_t::cmp_eq<ident_t>(ctx.negotiated_proto, CSTR("spdy/3.1")))
        spdy_version = spdy3_1;
    else
        throw exception_sys_t(log::error, EPROTO, "Unknown proto negotiated %.*s",
            (int)ctx.negotiated_proto.size(), ctx.negotiated_proto.ptr());

    log_debug("SPDY: version %d", spdy_version);

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.send_callback = &do_send;
    callbacks.on_ctrl_recv_callback = &on_ctrl_recv_callback;
    callbacks.on_data_recv_callback = &on_data_recv_callback;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session, spdy_version, &callbacks, this);
    return rv == 0;
}


bool spdy_framer_t::receive_data(in_t::ptr_t& in, unsigned int& res_code) {
    // recv_buffer = in;
    last_res_code = 0;
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
    res_code = last_res_code;
    return true;
}

bool spdy_framer_t::send_data(in_segment_t &out) {
    spdylay_session_send(session);
    out = send_buffer;
    send_buffer.clear();
    log_debug("SPDY: send_data %lu", out.size());
    return true;
}

}}}  // namespace phantom::io_benchmark::method_stream
