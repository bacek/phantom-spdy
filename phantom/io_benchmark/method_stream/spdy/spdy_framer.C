
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/exception.H>

#include <string.h>  // memset

#include "spdy_misc.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

namespace {

struct stream_data_t {
    inline stream_data_t(const in_segment_t &p)
        : post_data_str(in_t::ptr_t(p).__chunk()),
          post_data_ptr(post_data_str) {
    };

    str_t post_data_str;
    str_t::ptr_t post_data_ptr;
};

ssize_t read_callback(spdylay_session* UNUSED(session),
                     int32_t UNUSED(stream_id),
                     uint8_t* buf,
                     size_t length,
                     int* eof,
                     spdylay_data_source* source,
                     void* UNUSED(user_data)) {
    stream_data_t *sd = (static_cast<stream_data_t*>(source->ptr));
    if (!sd->post_data_ptr) {
        *eof = 1;
        return 0;
    }

    size_t to_copy = min(length, sd->post_data_ptr.size());
    out_t out((char*)buf, to_copy);
    out(sd->post_data_ptr);
    sd->post_data_ptr += to_copy;

    return to_copy;
}

}

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
    callbacks_.on_stream_close_callback = &on_stream_close_callback;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session_, spdy_version_, &callbacks_, this);
    if (rv != 0)
        return false;

    // Change WINDOW_SIZE to bloody large one to avoid additional control frames
    spdylay_settings_entry settings[] = {
        { SPDYLAY_SETTINGS_INITIAL_WINDOW_SIZE, 0, (1<<30U) - 1}
    };
    rv = spdylay_submit_settings(session_, 0, settings, 1);
    return rv == 0;
}


bool spdy_framer_t::receive_data(in_t::ptr_t& in, unsigned int& res_code) {
    last_res_code_ = 100;  // Fake "100 Continue"
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
                                  const char** nv,
                                  const in_segment_t& post_body) {
    stream_data_t* stream_data = nullptr;
    spdylay_data_provider data_prd;
    if (post_body) {
        stream_data = new stream_data_t(post_body);
        data_prd.source.ptr = stream_data;
        data_prd.read_callback = read_callback;
    }
    in_flight_requests_++;
    return spdylay_submit_request(
        session_, pri, nv, stream_data ? &data_prd : nullptr, stream_data);
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
}

void spdy_framer_t::on_stream_close_callback(
    spdylay_session* session,
    int32_t stream_id,
    spdylay_status_code status_code,
    void* user_data) {
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);
    log_debug("SPDY: Session closed %d with code %d (%ld)", stream_id, status_code, self->in_flight_requests_);

    stream_data_t *stream_data = static_cast<stream_data_t*>(
            spdylay_session_get_stream_user_data(session, stream_id));
    delete stream_data;
    self->in_flight_requests_--;
}

}}}  // namespace phantom::io_benchmark::method_stream
