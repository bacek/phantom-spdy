// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include <phantom/io_benchmark/method_stream/source.H>
#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/config_list.H>
#include <pd/base/string.H>

#include "spdy_transport.H"
#include "spdy_framer.H"

#include <ctype.h>

namespace phantom { namespace io_benchmark { namespace method_stream {

// Class to filter source data into SPDY request frames
// Basically it can parse some HTTP requests and generate SPDY requests from
// them.
class spdy_source_filter_t : public source_t {
public:
    // source_t implementation
    // Generate request. Returns false if source is exhausted
	virtual bool get_request(in_segment_t &request, in_segment_t &tag) const;

	virtual void do_init();
	virtual void do_run() const;
	virtual void do_stat_print() const;
	virtual void do_fini();
public:
    typedef method_stream::source_t source_t;
    struct config_t {
        config_binding_type_ref(source_t);

        config::objptr_t<source_t> source;
        config::list_t<string_t> headers;

        // Amount of concurrent requests to send
        // Default 1.
        ssize_t burst;

		void check(in_t::ptr_t const &) const;
    };

	spdy_source_filter_t(string_t const &name, config_t const &config);
    inline ~spdy_source_filter_t() {}

private:
    source_t& source;
    ssize_t burst;

    // Storage for predefined headers.
    // All headers stored in continuous memory block, \0 terminated.
    string_t nv_data_seg;
    sarray1_t<const char*> nv;
};

namespace {

// Aggregate all headers into single chunk of \0 separated values
string_t aggregator(const config::list_t<string_t> &headers) {
    string_t::ctor_t cons(1024);

    // Construct single buffer with all predefined headers.
    for (auto ptr = headers._ptr(); ptr; ++ptr) {
        cons(ptr.val());
        cons('\0');
    }

    return cons;
}

// Parse and aggregate headers into single storage
// Parsed headers are handled in next way:
// 1. Change to lower-case.
// 2. Replace "Host" with ":host"
// 3. Skip "Connection"
// 4. Remember "Content-Length" to pass POST body (if any)
string_t parse_headers(in_t::ptr_t& ptr, size_t& num_headers) {
    size_t limit = 4096;
    string_t::ctor_t storage_ctor(limit);

    num_headers = 0;
    while (true) {
        ++ptr;
        in_t::ptr_t start = ++ptr;  // skip '\r\n' on previous line

        // If it's empty line we have reached our destination
        if ((*ptr == '\n') || ((*ptr == '\r') && *(ptr + (size_t)1) == '\n'))
            break;

        if(!ptr.scan(":", 1, limit))
            throw exception_log_t(log::error, "Can't parse header name");

        in_segment_t header(start, ptr - start);
        MKCSTR(_header, header);
        log_debug("Found header '%s'", _header);

        in_t::ptr_t m = header;
        if (m.match<lower_t>(CSTR("host"))) {
            storage_ctor(CSTR(":host"));
        } else if (m.match<lower_t>(CSTR("content-length"))) {
            // Just skip it
            if(!ptr.scan("\n", 1, limit))
                throw exception_log_t(log::error, "Can't parse header value");
            continue;
        } else {
            for (char *p = _header; *p; ++p) {
                storage_ctor(tolower(*p));
            }
        }
        storage_ctor('\0');

        // Skip :
        ++ptr;
        // and spaces
        while (ptr.match<ident_t>(' '))
            ;
        start = ptr;
        if(!ptr.scan("\r\n", 1, limit))
            throw exception_log_t(log::error, "Can't parse header value");
        MKCSTR(value, in_segment_t(start, ptr - start));
        log_debug("Found value '%s'", value);
        storage_ctor(in_segment_t(start, ptr - start));
        storage_ctor('\0');

        num_headers++;
    }

    return storage_ctor;
}

}

spdy_source_filter_t::spdy_source_filter_t(string_t const& name,
                                           config_t const& config)
    : source_t(name),
      source(*config.source),
      burst(config.burst ? config.burst : 1),
      nv_data_seg(aggregator(config.headers)),
      nv(config.headers, [&](const string_t& h) {
            static const char* d = nv_data_seg.ptr();
            const char* ret = d;
            d += h.size() + 1;
            return ret;
      }) {
}

bool spdy_source_filter_t::get_request(in_segment_t& request,
                                       in_segment_t& tag) const {
    auto* framer = spdy_transport_t::current_framer();
    // Wait for framer.
    if (!framer) {
        tag = STRING("*skip*");
        return true;
    }

    // If it's fresh framer - start it.
    if (!framer->is_started()) {
        tag = STRING("*skip*");
        framer->start();
        return framer->send_data(request);
    }

    // Submit N requests
    while (framer->in_flight_requests() < burst) {
        in_segment_t original_request;
        if (!source.get_request(original_request, tag)) {
            // Wait for already in flight requests to finish
            log_debug("SPDY: original source is exhausted (%ld)", framer->in_flight_requests());
            if (framer->in_flight_requests())
                return framer->send_data(request);
            else
                return false;
        }

        in_t::ptr_t ptr = original_request;
        // Meh. operator bool actually initialize ptr...
        if (!ptr) {
            log_error("Meh!");
            return false;
        }

        // Parse generated request and produce SPDY request from it.
        // First line should be <method> <url> <version>
        in_t::ptr_t start = ptr;
        size_t limit = 4096;  // Up to 4k URLs.

        // We should switch to be able to send DATA frames. But this will work
        // for now
        if(!ptr.scan(" ", 1, limit))
            throw exception_log_t(log::error, "Can't parse METHOD");
        MKCSTR(method, in_segment_t(start, ptr - start));

        start = ++ptr;
        if(!ptr.scan(" ", 1, limit))
            throw exception_log_t(log::error, "Can't parse URL");
        MKCSTR(path, in_segment_t(start, ptr - start));

        start = ++ptr;
        if(!ptr.scan("\r\n", 1, limit))
            throw exception_log_t(log::error, "Can't parse VERSION");
        MKCSTR(version, in_segment_t(start, ptr - start));

        size_t num_headers;
        string_t headers_storage = parse_headers(ptr, num_headers);

        // 8 for "standard" headers
        // +1 for nullptr
        const char *nv_send[8 + num_headers + 1];
        nv_send[0] = ":method";     nv_send[1] = method;
        nv_send[2] = ":version";    nv_send[3] = version;
        nv_send[4] = ":path";       nv_send[5] = path;
        nv_send[6] = ":scheme";     nv_send[7] = "https";

        const char ** nv_ptr = nv_send + 8;

        // Propagate pointers into NV
        const char *data = headers_storage.ptr();
        start = headers_storage;
        in_t::ptr_t tmp = start;
        while (tmp.scan("\0", 1, limit)) {
            *nv_ptr++ = data;
            data += tmp - start + 1;
            start = ++tmp;
        }

        *nv_ptr = nullptr;

        int rv = framer->submit_request(0, nv_send, nullptr);
        if (rv != 0)
            return false;
    }

    return framer->send_data(request);
}


void spdy_source_filter_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!source)
        config::error(ptr, "Original 'source' is required for spdy_source_filter");
}

void spdy_source_filter_t::do_init() {
    source.init(name);
}

void spdy_source_filter_t::do_run() const {
    source.run(name);
}

void spdy_source_filter_t::do_stat_print() const {
    source.stat_print(name);
}

void spdy_source_filter_t::do_fini() {
    source.fini(name);
}

}}}  // namespace phantom::io_benchmark::method_stream

namespace pd { namespace config {
using phantom::io_benchmark::method_stream::spdy_source_filter_t;
using phantom::io_benchmark::method_stream::source_t;

config_binding_sname(spdy_source_filter_t);
config_binding_value(spdy_source_filter_t, source);
config_binding_value(spdy_source_filter_t, headers);
config_binding_value(spdy_source_filter_t, burst);

config_binding_cast(spdy_source_filter_t, source_t);
config_binding_ctor(source_t, spdy_source_filter_t);
}}

