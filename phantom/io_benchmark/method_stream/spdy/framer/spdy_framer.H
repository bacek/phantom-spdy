
#pragma once

#include <phantom/shared.H>
#include <pd/base/config.H>

#pragma GCC visibility push(default)

namespace phantom { namespace io_benchmark {

// "SPDY protocol framer"
// Wrapper around spdylay/nghttp2 which provides shared state between various
// components for SPDY module
class spdy_framer_t {
public:
    struct config_t {
        void check(in_t::ptr_t const &) const {};
    };

    spdy_framer_t();
    spdy_framer_t(string_t const &, config_t const &);
	~spdy_framer_t();
};

}}  // namespace phantom::io_benchmark

#pragma GCC visibility pop
