
#pragma once

#include <phantom/shared.H>

#pragma GCC visibility push(default)

namespace phantom { namespace io_benchmark { namespace method_stream {

// "SPDY protocol framer"
// Wrapper around spdylay/nghttp2 which provides shared state between various
// components for SPDY module
class spdy_framer_t : public shared_t {
public:

protected:
	spdy_framer_t(string_t const &name) throw();
	inline ~spdy_framer_t() throw() { }

};

}}}  // namespace phantom::io_benchmark::method_stream

#pragma GCC visibility pop
