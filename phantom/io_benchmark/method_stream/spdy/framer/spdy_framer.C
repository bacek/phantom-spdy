
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>

namespace phantom { namespace io_benchmark {

MODULE(io_benchmark_method_stream_spdy_framer);

namespace method_stream {
config_binding_sname(spdy_framer_t);
config_binding_value(spdy_framer_t, spdy_version);
config_binding_ctor(spdy_framer_t, spdy_framer_t);
}

config_enum_internal_sname(spdy_framer_t, spdy_version_t);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy2);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy3);
config_enum_internal_value(spdy_framer_t, spdy_version_t, spdy3_1);

namespace {
ssize_t do_send(spdylay_session* session,
            const uint8_t* data, size_t length, int flags,
            void *user_data) {
    (void)session;
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);
    return self->send_callback(data, length, flags);
}

ssize_t do_receive(spdylay_session* session,
               uint8_t* buf, size_t length, int flags,
               void* user_data) {
    (void)session;
    spdy_framer_t* self = static_cast<spdy_framer_t*>(user_data);
    return self->recv_callback(buf, length, flags);
}

}

spdy_framer_t::spdy_framer_t(string_t const &, config_t const &config)
    : spdy_version(config.spdy_version) {
}

spdy_framer_t::~spdy_framer_t() {}

bool spdy_framer_t::start(recv_callback_t rc, send_callback_t sc) {
    recv_callback = std::move(rc);
    send_callback = std::move(sc);

    callbacks.send_callback = &do_send;
    callbacks.recv_callback = &do_receive;

    // TODO Add more callbacks

    int rv = spdylay_session_client_new(&session, spdy_version, &callbacks, this);
    return rv == 0;
}


void spdy_framer_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!spdy_version)
        config::error(ptr, "'spdy_version' is required. Try to put 'spdy3_1'");
}

}}  // namespace phantom::io_benchmark
