
#pragma once

#include <phantom/shared.H>
#include <pd/base/config.H>
#include <pd/base/config_enum.H>

#include <spdylay/spdylay.h>

#include <functional>

#pragma GCC visibility push(default)

namespace phantom { namespace io_benchmark {

// "SPDY protocol framer"
// Wrapper around spdylay/nghttp2 which provides shared state between various
// components for SPDY module
class spdy_framer_t {
public:
    // Values are same as in spdylay
    enum spdy_version_t {
        unconfigured = 0,
        spdy2 = 2,
        spdy3 = 3,
        spdy3_1 = 4,
    };

    struct config_t {
        // Because Phantom ask source before establishing connection we have to
        // predefine used proto
        config::enum_t<spdy_version_t> spdy_version;

        config_t() : spdy_version(unconfigured) {}

        void check(in_t::ptr_t const &) const;
    };

    spdy_framer_t(string_t const &, config_t const &);
	~spdy_framer_t();

    // Prepare callbacks, create spdy session.
    // Returns true if everything is ok.
    bool start();

    // Receive data from the other end.
    // Dta to receive will be read from "in"
    bool receive_data(in_t::ptr_t &in); // const?

    // Send data to the other end.
    // Data to send will be filled into "out"
    bool send_data(in_segment_t &out); // const?

public:
    // Preconfigured SPDY version
    spdy_version_t spdy_version;

    // Actual SPDY implementation
    spdylay_session* session;

public:
    spdylay_session_callbacks callbacks;

    in_segment_t send_buffer;
};

}}  // namespace phantom::io_benchmark

#pragma GCC visibility pop
