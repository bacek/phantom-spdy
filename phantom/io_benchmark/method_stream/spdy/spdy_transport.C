// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "spdy_transport.H"

#include <pd/base/config.H>
#include <phantom/module.H>

namespace phantom { namespace io_benchmark { namespace method_stream {

MODULE(io_benchmark_method_stream_spdy);

namespace {
    // Thread local SPDY framer
    bq_spec_decl(spdy_framer_t, cur_framer);

    // Connection for SPDY.
    // It's created thread local by method_stream_t.
    // So we piggy back on it and create SPDY framer alongside.
	class conn_ssl_t : public conn_t {
		bq_conn_ssl_t bq_conn_ssl;

		virtual operator bq_conn_t &() {
            return bq_conn_ssl;
        }

	public:
		inline conn_ssl_t(
			int fd, ssl_ctx_t const &ctx, interval_t _timeout, fd_ctl_t const *_ctl
		) : conn_t(fd), bq_conn_ssl(fd, ctx, _timeout, _ctl) {
            // Reset framer for new con
            delete cur_framer;
            cur_framer = new spdy_framer_t(ctx);
        }

		virtual ~conn_ssl_t() throw() {
            delete cur_framer;
        }
	};
}

spdy_transport_t::~spdy_transport_t() throw() {}

conn_t *spdy_transport_t::new_connect(int fd, fd_ctl_t const *_ctl) const {
    return new conn_ssl_t(fd, ctx, timeout, _ctl);
}

spdy_framer_t* spdy_transport_t::current_framer() {
    return cur_framer;
}

}}}  // namespace phantom::io_benchmark::method_stream

namespace pd { namespace config {

using phantom::io_benchmark::method_stream::spdy_transport_t;
using phantom::io_benchmark::method_stream::transport_t;

config_binding_sname(spdy_transport_t);
config_binding_type(spdy_transport_t, auth_t);
config_binding_value(spdy_transport_t, auth);
config_binding_value(spdy_transport_t, ciphers);
config_binding_value(spdy_transport_t, timeout);
config_binding_value(spdy_transport_t, protos);
config_binding_cast(spdy_transport_t, transport_t);
config_binding_ctor(transport_t, spdy_transport_t);

}}  // namespace pd::config

