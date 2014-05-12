#include "spdy_proto.H"

#include <pd/base/config.H>

#include "transport_spdy.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

spdy_proto_t::spdy_proto_t(string_t const &sname, config_t const &)
    : proto_t(sname) {
}

bool spdy_proto_t::reply_parse(in_t::ptr_t& ptr,
                               in_segment_t const& request,
                               unsigned int& res_code,
                               logger_t::level_t& lev) const {
    (void)request;
    auto* framer = transport_spdy_t::current_framer();
    assert(framer);

    log_debug("SPDY: proto");

    // If it's fresh framer there is nothing to receive.
    if (!framer->session) {
        return true;
    }

    if (!framer->receive_data(ptr))
        return false;

    res_code = -1;
    // res_code = 200;
    lev = logger_t::all;

    return true;
}


void spdy_proto_t::config_t::check(in_t::ptr_t const &) const {
    //if (!framer)
    //    config::error(ptr, "SPDY 'framer' is required for spdy_proto_t");
}

}}}  // namespace phantom::io_benchmark::method_stream

namespace pd { namespace config {
using phantom::io_benchmark::method_stream::spdy_proto_t;
using phantom::io_benchmark::method_stream::proto_t;

config_binding_sname(spdy_proto_t);

config_binding_cast(spdy_proto_t, proto_t);
config_binding_ctor(proto_t, spdy_proto_t);
}}

