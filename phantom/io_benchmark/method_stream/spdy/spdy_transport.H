// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_TRANSPORT_H_
#define PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_TRANSPORT_H_

#include <phantom/io_benchmark/method_stream/transport.H>

#include <phantom/ssl/auth.H>

#include <pd/ssl/bq_conn_ssl.H>

#include <pd/bq/bq_util.H>
#include <pd/bq/bq_spec.H>

#include <pd/base/config.H>
#include <pd/base/config_list.H>
#include <pd/base/fd.H>
#include <pd/base/log.H>

#include <unistd.h>

#include "spdy_framer.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

// SPDY transport.
// Creates connection and associated spdy_framer_t alongside.
// framer is stored in TLS, same as connection in method_stream_t.
// Code is deliberately borrowed from Phantom's transport_ssl_t.
class spdy_transport_t : public transport_t {
public:
	typedef ssl::auth_t auth_t;
	struct config_t {
		config_binding_type_ref(auth_t);
		config::objptr_t<auth_t> auth;
		string_t ciphers;
		interval_t timeout;
		config::list_t<string_t> protos;

		inline config_t() : auth(), ciphers(), timeout(interval::second), protos() { }

		inline ~config_t() throw() { }

		inline void check(in_t::ptr_t const &) const;
	};

	inline spdy_transport_t(string_t const &, config_t const &config) :
		transport_t(), ctx(ssl_ctx_t::client, config.auth, config.ciphers, config.protos),
		timeout(config.timeout) { }

	~spdy_transport_t() throw();

	virtual conn_t *new_connect(int fd, fd_ctl_t const *_ctl) const;

    // Accessor to current spdy_framer
    static spdy_framer_t* current_framer();

protected:
	ssl_ctx_t ctx;
	interval_t timeout;
};


}}}  // namespace yandex

#endif  // PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_TRANSPORT_H_

