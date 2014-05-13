// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_PROTO_H_
#define PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_PROTO_H_

#include <phantom/io_benchmark/method_stream/proto.H>
#include <phantom/io_benchmark/method_stream/logger.H>

#include "spdy_framer.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

class spdy_proto_t : public proto_t {
public:
    // We have to have "config" for loadable module.
    struct config_t {
		void check(in_t::ptr_t const &) const {}
    };

    // proto_t implementation
	virtual bool reply_parse(
		in_t::ptr_t &ptr, in_segment_t const &request,
		unsigned int &res_code, logger_t::level_t &lev
	) const;

    // shared_t implementation
	virtual void do_init() {};
	virtual void do_run() const {};
	virtual void do_stat_print() const {};
	virtual void do_fini() {};

    spdy_proto_t(string_t const &sname, config_t const &config);
};


}}}  // namespace phantom::io_benchmark::method_stream

#endif  // PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_PROTO_H_

