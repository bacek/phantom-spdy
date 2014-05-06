
#include "spdy_framer.H"

#include <phantom/module.H>
#include <pd/base/config.H>

namespace phantom { namespace io_benchmark { namespace method_stream {

MODULE(io_benchmark_method_stream_spdy_framer);

namespace spdy_framer{
config_binding_cast(spdy_framer_t, shared_t);
//config_binding_ctor(spdy_framer_t, spdy_framer_t);
}

spdy_framer_t::spdy_framer_t(string_t const& name) throw() :
    shared_t(name) {
}


}}}  // namespace phantom::io_benchmark::method_stream
