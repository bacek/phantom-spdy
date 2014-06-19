# phantom-spdy

SPDY module for Phantom.

## Overview

SPDY Phantom plugin contains few elements to work with SPDY servers.

This module requires latest version of Phantom with [NPN support patch](https://github.com/mamchits/phantom/pull/9)

* spdy_transport_t to establish SPDY connection and negotiate protocol to
  use.
* spdy_source_filter_t to parse original HTTP requests and convert it to
  SPDY frames.
* spdy_proto_t to do handle responses from SPDY servers.

## Configuration

Example configuration is available in [examples/io_benchmark_method_stream_proto_http.conf](https://github.com/bacek/phantom-spdy/blob/master/examples/io_benchmark_method_stream_proto_http.conf)

### spdy_transport_t

spdy_transport_t accepts next parameters

* protos - list of protocols to negotiate via NPN.

Example:

        # Special SPDY transport
        transport_t ssl_transport = spdy_transport_t {
            protos = { "spdy/3.1" "spdy/3" "spdy/2" }
        }

### spdy_source_filter_t

spdy_source_filter_t converts original HTTP requests into SPDY frames

Parameters:

* source - mandatory parameter for original requests. Can be anything supported
  by Phantom. For example source_log_t which parses file with requests.

* burst - amount of in-flight requests to produce. Default 1.

* headers - list of additional headers to add to each request.

Example:

        source_t source_log = source_log_t {
            filename = "ammo.stpd"
        }

        source_t spdy_source = spdy_source_filter_t {
            # Original URLs source.
            source = source_log

            # Amount of parallel requests in single connection
            burst = 1

            # Predefined set of headers to send.
            # spdy_source_filter_t will append :path from .source
            headers = {
                "accept-encoding"   "gzip,deflate"
                "accept"            "*/*"
            }
        }

### spdy_proto_t

spdy_proto_t parse responses from SPDY server and doesn't require any
configuration.


## Installation

### spdylay

SPDY support is implement in [spdylay module](https://github.com/tatsuhiro-t/spdylay). So you have to install it
first.

### Phantom

Get latest source of [Phantom](https://github.com/mamchits/phantom), apply patch
for NPN negotiation, build and install.

Alternatively grab sources from [my fork](https://github.com/bacek/phantom/tree/spdy_support_2)

### Module it self

With default Phantom installation running "make -R" should be sufficient to
build module.
