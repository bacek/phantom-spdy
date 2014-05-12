
#include <vector>

#include <phantom/io_benchmark/method_stream/source.H>
#include <phantom/module.H>
#include <pd/base/config.H>
#include <pd/base/config_list.H>

#include "transport_spdy.H"
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

    // It's awful hack...
    std::vector<char*> nv;
    std::vector<std::vector<char>> nv_data;
};

spdy_source_filter_t::spdy_source_filter_t(string_t const& name,
                                           config_t const& config)
    : source_t(name),
      source(*config.source),
      burst(config.burst ? config.burst : 1) {

    for (auto ptr = config.headers._ptr(); ptr; ++ptr) {
        nv_data.emplace_back(ptr.val().ptr(), ptr.val().ptr() + ptr.val().size());
        nv_data.rbegin()->push_back('\0');
        nv.push_back(nv_data.back().data());
    }
}

bool spdy_source_filter_t::get_request(in_segment_t& request,
                                       in_segment_t& tag) const {
    log_debug("SPDY: source filter");

    auto* framer = transport_spdy_t::current_framer();
    log_debug("SPDY: framer %lx", framer);
    // Wait for framer.
    if (!framer)
        return true;

    // If it's fresh framer - start it.
    if (!framer->session) {
        framer->start();
        return framer->send_data(request);
    }

    // Submit N requests
    while (framer->in_flight_requests < burst) {
        in_segment_t original_request;
        if (!source.get_request(original_request, tag)) {
            // Wait for already in flight requests to finish
            log_debug("SPDY: original source is exhausted (%ld)", framer->in_flight_requests);
            if (framer->in_flight_requests)
                return framer->send_data(request);
            else
                return false;
        }

        // Assume that request is just url to fetch.
        // TODO Implement proper parsing. Yours truly, CO.
        // +2 for ':path' and path
        // +1 for nullptr
        const char *nv_send[nv.size() + 3];
        nv_send[0] = ":path";

        in_t::ptr_t ptr = original_request;
        // Meh. operator bool actually initialize ptr...
        if (!ptr) {
            log_error("Meh!");
            return false;
        }
        auto p = ptr.__chunk();
        std::vector<char> ps { p.ptr(), p.ptr() + p.size() };
        ps.push_back('\0');
        nv_send[1] = ps.data();

        //nv_send[1] = original_request
        std::copy(nv.begin(), nv.end(), nv_send + 2);
        nv_send[nv.size() + 2] = nullptr;

        log_debug("SPDY: submitting request");
        int rv = spdylay_submit_request(framer->session, 0, nv_send, nullptr, nullptr);
        if (rv != 0)
            return false;
        framer->in_flight_requests++;
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

