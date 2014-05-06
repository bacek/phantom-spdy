
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

spdy_framer_t::spdy_framer_t() {}

spdy_framer_t::spdy_framer_t(string_t const &, config_t const &config)
    : spdy_version(config.spdy_version) {
}

spdy_framer_t::~spdy_framer_t() {}



void spdy_framer_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!spdy_version)
        config::error(ptr, "'spdy_version' is required. Try to put 'spdy3_1'");
}

}}  // namespace phantom::io_benchmark
