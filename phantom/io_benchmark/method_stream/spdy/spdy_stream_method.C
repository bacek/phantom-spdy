// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include <phantom/io_benchmark/method_stream/method_stream.H>
#include <pd/base/config.H>

#include "framer/spdy_framer.H"

namespace phantom { namespace io_benchmark {

// SPDY wrapper for method_t to hold additional variables. E.g. "framer"
class spdy_method_stream_t : public method_t {
public:
    // typedef required for config magic
    typedef io_benchmark::spdy_framer_t spdy_framer_t;
    typedef io_benchmark::method_t method_t;
    struct config_t {
        config_binding_type_ref(spdy_framer_t);
        config_binding_type_ref(method_t);

        config::objptr_t<spdy_framer_t> framer;
        config::objptr_t<method_t> method;

        void check(in_t::ptr_t const &) const;
    };

    // shared_t implementation
	virtual void do_init();
	virtual void do_run() const;
	virtual void do_stat_print() const;
	virtual void do_fini();

    // method_t implementaion
	virtual bool test(times_t &times) const;

    spdy_method_stream_t(string_t const &sname, config_t const& config);
    ~spdy_method_stream_t();

private:
    // Original method to wrap
    method_t& method;

    spdy_framer_t& framer;
};




// config binding
namespace method_stream {
config_binding_sname(spdy_method_stream_t);

config_binding_type(spdy_method_stream_t, method_t);
config_binding_value(spdy_method_stream_t, method);
config_binding_type(spdy_method_stream_t, spdy_framer_t);
config_binding_value(spdy_method_stream_t, framer);

config_binding_cast(spdy_method_stream_t, method_t);
config_binding_ctor(method_t, spdy_method_stream_t);
}
// end of config binding


spdy_method_stream_t::spdy_method_stream_t(string_t const& sname, config_t const &config)
    : method_t(sname), method(*config.method), framer(*config.framer) {

    spdy_framer_t::recv_callback_t rv;
    spdy_framer_t::send_callback_t sn;
    framer.start(rv, sn);
}

spdy_method_stream_t::~spdy_method_stream_t() {
}


void spdy_method_stream_t::do_init() {}
void spdy_method_stream_t::do_run() const {}
void spdy_method_stream_t::do_stat_print() const {}
void spdy_method_stream_t::do_fini() {}


bool spdy_method_stream_t::test(times_t &times) const {
    return method.test(times);
}



void spdy_method_stream_t::config_t::check(in_t::ptr_t const &ptr) const {
    if (!method)
        config::error(ptr, "'method' is required");
    if (!framer)
        config::error(ptr, "'framer' is required");
}

}}  // namespace phantom::io_benchmark
