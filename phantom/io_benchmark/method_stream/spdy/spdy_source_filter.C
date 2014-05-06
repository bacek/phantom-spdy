
#include <phantom/io_benchmark/method_stream/source.H>
#include <phantom/module.H>
#include <pd/base/config.H>

#include "framer/spdy_framer.H"

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
    // typedef required for config magic
    typedef io_benchmark::spdy_framer_t spdy_framer_t;
    typedef method_stream::source_t source_t;
    struct config_t {
        config_binding_type_ref(spdy_framer_t);
        config_binding_type_ref(source_t);

        config::objptr_t<spdy_framer_t> framer;
        config::objptr_t<source_t> source;

        config_t() throw() : framer() {}

		void check(in_t::ptr_t const &) const;
    };

	spdy_source_filter_t(string_t const &name, config_t const &config);
    inline ~spdy_source_filter_t() {}

private:
    spdy_framer_t* framer;
    source_t& source;
};

namespace spdy_source_filter {
config_binding_sname(spdy_source_filter_t);
config_binding_value(spdy_source_filter_t, framer);
config_binding_value(spdy_source_filter_t, source);

config_binding_cast(spdy_source_filter_t, source_t);
config_binding_ctor(source_t, spdy_source_filter_t);
}

spdy_source_filter_t::spdy_source_filter_t(string_t const &name, config_t const &config) :
    source_t(name), source(*config.source) {
}

bool spdy_source_filter_t::get_request(in_segment_t& request,
                                       in_segment_t& tag) const {
    log_debug("source filter");
    return source.get_request(request, tag);
}


void spdy_source_filter_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!source)
        config::error(ptr, "Original 'source' is required for spdy_source_filter");
    if (!framer)
        config::error(ptr, "SPDY 'framer' is required for spdy_source_filter");
}

}}}  // namespace phantom::io_benchmark::method_stream
