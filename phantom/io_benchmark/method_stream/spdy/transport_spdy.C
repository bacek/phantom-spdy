// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "transport_spdy.H"

#include <pd/base/config.H>
#include <phantom/module.H>

namespace phantom { namespace io_benchmark { namespace method_stream {

MODULE(io_benchmark_method_stream_spdy);

namespace transport_spdy {
config_binding_sname(transport_spdy_t);
config_binding_type(transport_spdy_t, auth_t);
config_binding_value(transport_spdy_t, auth);
config_binding_value(transport_spdy_t, ciphers);
config_binding_value(transport_spdy_t, timeout);
config_binding_value(transport_spdy_t, protos);
config_binding_cast(transport_spdy_t, transport_t);
config_binding_ctor(transport_t, transport_spdy_t);
}

namespace {
    __thread spdy_framer_t* cur_framer;
}

transport_spdy_t::~transport_spdy_t() throw() {
    delete cur_framer;
}

conn_t *transport_spdy_t::new_connect(int fd, fd_ctl_t const *_ctl) const {
    auto* res = new conn_ssl_t(fd, ctx, timeout, _ctl);
    // Reset framer for new con
    delete cur_framer;
    cur_framer = new spdy_framer_t(ctx);
    log_debug("SPDY: creating framer %lx", cur_framer);
	return res;
}

transport_spdy_t::conn_ssl_t::operator bq_conn_t &() {
	return bq_conn_ssl;
}

transport_spdy_t::conn_ssl_t::~conn_ssl_t() throw() { }

spdy_framer_t* transport_spdy_t::current_framer() {
    return cur_framer;
}

}}}  // namespace yandex
