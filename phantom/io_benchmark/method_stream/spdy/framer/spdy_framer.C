
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>

namespace phantom { namespace io_benchmark {

namespace method_stream {
config_binding_sname(spdy_framer_t);
config_binding_ctor(spdy_framer_t, spdy_framer_t);
}

MODULE(io_benchmark_method_stream_spdy_framer);

spdy_framer_t::spdy_framer_t() {}

spdy_framer_t::spdy_framer_t(string_t const &, config_t const &) {}

spdy_framer_t::~spdy_framer_t() {}

}}  // namespace phantom::io_benchmark
