// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include <pd/base/config.H>

#include <phantom/io_benchmark/method_stream/proto.H>
#include <phantom/io_benchmark/method_stream/logger.H>


#include "spdy_framer.H"
#include "spdy_transport.H"
#include "spdy_misc.H"

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

spdy_proto_t::spdy_proto_t(string_t const &sname, config_t const &)
    : proto_t(sname) {
}

bool spdy_proto_t::reply_parse(in_t::ptr_t& ptr,
                               in_segment_t const& UNUSED(request),
                               unsigned int& res_code,
                               logger_t::level_t& lev) const {
    auto* framer = spdy_transport_t::current_framer();
    assert(framer);

    // If it's fresh framer there is nothing to receive.
    if (!framer->is_started()) {
        return true;
    }

    if (!framer->receive_data(ptr, res_code))
        return false;

    if(res_code >= 200 && res_code < 400)
		lev = logger_t::all;
	else if(res_code >= 400 && res_code < 500)
		lev = logger_t::proto_warning;
	else
		lev = logger_t::proto_error;

    return true;
}

}}}  // namespace phantom::io_benchmark::method_stream

namespace pd { namespace config {

using phantom::io_benchmark::method_stream::spdy_proto_t;
using phantom::io_benchmark::method_stream::proto_t;

config_binding_sname(spdy_proto_t);

config_binding_cast(spdy_proto_t, proto_t);
config_binding_ctor(proto_t, spdy_proto_t);
}}

