
#pragma once

#include <phantom/shared.H>
#include <pd/base/config.H>
#include <pd/base/config_enum.H>
#include <pd/ssl/ssl.H>

#include <spdylay/spdylay.h>

#include <functional>

#pragma GCC visibility push(default)

namespace phantom { namespace io_benchmark { namespace method_stream {

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

    spdy_framer_t(const ssl_ctx_t& ctx);
	~spdy_framer_t();

    // Prepare callbacks, create spdy session.
    // Returns true if everything is ok.
    bool start();

    bool is_started() const {
        return session != nullptr;
    }

    // Receive data from the other end.
    // Data to receive will be read from "in"
    bool receive_data(in_t::ptr_t &in, unsigned int& res_code); // const?

    // Send data to the other end.
    // Data to send will be filled into "out"
    bool send_data(in_segment_t &out); // const?

    // Amount of requests in flight right now
    // ssize_t in_flight_requests() const;

public:
    // Preconfigured SPDY version
    spdy_version_t spdy_version;

    const ssl_ctx_t& ctx;

    // Actual SPDY implementation
    spdylay_session* session;

    // Amount of requests in flight right now
    ssize_t in_flight_requests;

protected:
    spdylay_session_callbacks callbacks;

    in_segment_list_t send_buffer;
    unsigned int last_res_code;

    // spdylay callbacks
    static ssize_t do_send(spdylay_session* session,
            const uint8_t* data, size_t length, int flags,
            void *user_data);

    static void on_ctrl_recv_callback(spdylay_session* session,
                              spdylay_frame_type type,
                              spdylay_frame* frame,
                              void* user_data);

    static void on_data_recv_callback(spdylay_session* session,
                           uint8_t flags,
                           int32_t stream_id,
                           int32_t length,
                           void* user_data);
};

}}}  // namespace phantom::io_benchmark::method_stream

#pragma GCC visibility pop
