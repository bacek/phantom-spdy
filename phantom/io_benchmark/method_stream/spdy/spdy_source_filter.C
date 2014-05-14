// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include <phantom/io_benchmark/method_stream/source.H>
#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/config_list.H>

#include "spdy_transport.H"
#include "spdy_framer.H"

namespace phantom { namespace io_benchmark { namespace method_stream {

// Class to filter source data into SPDY request frames
// Basically it can parse some HTTP requests and generate SPDY requests from
// them.
class spdy_source_filter_t : public source_t {
public:
    // source_t implementation
    // Generate request. Returns false if source is exhausted
	virtual bool get_request(in_segment_t &request, in_segment_t &tag) const;

	virtual void do_init() { }
	virtual void do_run() const { }
	virtual void do_stat_print() const { }
	virtual void do_fini() { }
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
    if (!framer)
        return true;

    // If it's fresh framer - start it.
    if (!framer->is_started()) {
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

        // Assume that request is just url to fetch.
        // TODO Implement proper parsing. Yours truly, CO.
        // +2 for ':path' and path
        // +1 for nullptr
        const char *nv_send[nv.size + 3];
        nv_send[0] = ":path";

        // Construct actual "path"
        string_t::ctor_t cons(original_request.size());
        cons(ptr, ptr.__chunk().size());
        cons('\0');
        string_t path = cons;
        nv_send[1] = path.ptr();

        memcpy(nv_send + 2, nv.items, nv.size * sizeof(nv.items[0]));
        nv_send[nv.size + 2] = nullptr;

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

