
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>


namespace pd { namespace config {

using phantom::io_benchmark::spdy_framer_t;

config_binding_sname(spdy_framer_t);
config_binding_value(spdy_framer_t, spdy_version);
config_binding_ctor(spdy_framer_t, spdy_framer_t);

config_enum_internal_sname(spdy_framer_t, spdy_version_t);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy2);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy3);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy3_1);

}}

namespace phantom { namespace io_benchmark {

MODULE(io_benchmark_method_stream_spdy_framer);


namespace {
ssize_t do_send(spdylay_session* session,
            const uint8_t* data, size_t length, int flags,
            void *user_data) {
    (void)session;
    (void)flags;
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);

    string_t::ctor_t buf(length);
    for (size_t i = 0; i < length; ++i)
        buf(*data++);
    string_t s = buf;
    in_segment_list_t t;
    t.append(s);
    self->send_buffer = t;

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
    log_debug("SPDY: Got DATA frame for stream %d length %d", stream_id, length);
}

}

spdy_framer_t::spdy_framer_t(string_t const &, config_t const &config)
    : spdy_version(config.spdy_version) {
}

spdy_framer_t::~spdy_framer_t() {}

bool spdy_framer_t::start() {

    callbacks.send_callback = &do_send;
    callbacks.on_ctrl_recv_callback = &on_ctrl_recv_callback;
    callbacks.on_data_recv_callback = &on_data_recv_callback;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session, spdy_version, &callbacks, this);
    return rv == 0;
}


bool spdy_framer_t::receive_data(in_t::ptr_t& in) {
    log_debug("SPDY: receive_data");
    // recv_buffer = in;
    if (!in)
        return false;
    str_t s = in.__chunk();
    while (s.size() > 0) {
        int processed = spdylay_session_mem_recv(
            session, reinterpret_cast<const uint8_t*>(s.ptr()), s.size());
        log_debug("SPDY: receive_data chunk %d", processed);
        if (processed < 0)
            return false;
        s = str_t(s.ptr() + processed, s.size() - processed);
    }
    return true;
}

bool spdy_framer_t::send_data(in_segment_t &out) {
    spdylay_session_send(session);
    out = send_buffer;
    log_debug("SPDY: send_data %lu", out.size());
    return true;
}


void spdy_framer_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!spdy_version)
        config::error(ptr, "'spdy_version' is required. Try to put 'spdy3_1'");
}

}}  // namespace phantom::io_benchmark
