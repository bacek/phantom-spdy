
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/exception.H>

#include <string.h>  // memset

#include "spdy_misc.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

spdy_framer_t::spdy_framer_t(const ssl_ctx_t& c)
    : spdy_version_(spdy3_1),
      ctx_(c),
      session_(nullptr),
      in_flight_requests_(0) {}

spdy_framer_t::~spdy_framer_t() {
    spdylay_session_del(session_);
}

bool spdy_framer_t::start() {
    log_debug("SPDY: starting framer %lx", this);

    if (string_t::cmp_eq<ident_t>(ctx_.negotiated_proto, CSTR("spdy/2")))
        spdy_version_ = spdy2;
    else if (string_t::cmp_eq<ident_t>(ctx_.negotiated_proto, CSTR("spdy/3")))
        spdy_version_ = spdy3;
    else if (string_t::cmp_eq<ident_t>(ctx_.negotiated_proto, CSTR("spdy/3.1")))
        spdy_version_ = spdy3_1;
    else
        throw exception_sys_t(log::error, EPROTO, "Unknown proto negotiated %.*s",
            (int)ctx_.negotiated_proto.size(), ctx_.negotiated_proto.ptr());

    log_debug("SPDY: version %d", spdy_version_);

    memset(&callbacks_, 0, sizeof(callbacks_));

    callbacks_.send_callback = &do_send;
    callbacks_.on_ctrl_recv_callback = &on_ctrl_recv_callback;
    callbacks_.on_data_recv_callback = &on_data_recv_callback;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session_, spdy_version_, &callbacks_, this);
    return rv == 0;
}


bool spdy_framer_t::receive_data(in_t::ptr_t& in, unsigned int& res_code) {
    last_res_code_ = 0;
    if (!in)
        return false;
    str_t s = in.__chunk();
    log_debug("SPDY: receive_data %ld", s.size());
    while (s.size() > 0) {
        int processed = spdylay_session_mem_recv(
            session_, reinterpret_cast<const uint8_t*>(s.ptr()), s.size());
        if (processed < 0)
            return false;
        s = str_t(s.ptr() + processed, s.size() - processed);
    }
    res_code = last_res_code_;
    return true;
}

bool spdy_framer_t::send_data(in_segment_t &out) {
    spdylay_session_send(session_);
    out = send_buffer_;
    send_buffer_.clear();
    log_debug("SPDY: send_data %lu", out.size());
    return true;
}

int spdy_framer_t::submit_request(uint8_t pri,
                       const char **nv,
                       void *stream_user_data) {
    in_flight_requests_++;
    return spdylay_submit_request(session_, pri, nv, nullptr, stream_user_data);
}

ssize_t spdy_framer_t::do_send(spdylay_session* UNUSED(session),
            const uint8_t* data, size_t length, int UNUSED(flags),
            void *user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    string_t::ctor_t buf(length);
    for (size_t i = 0; i < length; ++i)
        buf(*data++);
    string_t s = buf;
    self->send_buffer_.append(s);

    return length;
}

void spdy_framer_t::on_ctrl_recv_callback(spdylay_session* UNUSED(session),
                              spdylay_frame_type type,
                              spdylay_frame* frame,
                              void* user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    log_debug("SPDY: Got CONTROL frame type %d %d %ld",
        type,
        frame->ctrl.hd.flags,
        self->in_flight_requests_);

    if (type == SPDYLAY_SYN_REPLY) {
        if (frame->syn_reply.hd.flags & SPDYLAY_CTRL_FLAG_FIN)
            self->in_flight_requests_--;

        // Search for :status and extract it
        // Iterate over even headers.
        for (ssize_t i = 0; frame->syn_reply.nv[i]; i +=2 ) {
            if (strncmp(frame->syn_reply.nv[i], ":status", 7) == 0) {
                char* end;
                self->last_res_code_ = strtoul(frame->syn_reply.nv[i+1], &end, 10);
                if (*end != ' ') {
                    log_debug("Failed to parse status line");
                    self->last_res_code_ = 500;
                }
                log_debug("SPDY: status '%d'", self->last_res_code_);
                break;
            }
        }
    } else if (type == SPDYLAY_GOAWAY || type == SPDYLAY_RST_STREAM) {
        // It's not actually true. But it will work for now
        self->last_res_code_ = 500;
        self->in_flight_requests_--;
    }
}

void spdy_framer_t::on_data_recv_callback(spdylay_session* UNUSED(session),
                           uint8_t flags,
                           int32_t stream_id,
                           int32_t length,
                           void* user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);
    log_debug("SPDY: Got DATA frame for stream %d length %d flags %d (%ld)",
            stream_id,
            length,
            flags,
            self->in_flight_requests_);
    if (flags & SPDYLAY_DATA_FLAG_FIN)
        self->in_flight_requests_--;
}

}}}  // namespace phantom::io_benchmark::method_stream
