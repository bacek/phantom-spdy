include /usr/share/phantom/module.mk

# ===
# MODULE,dir,package_suffix,local_libs,nonlocal_libs

$(eval $(call MODULE,io_benchmark/method_stream/spdy,_ssl,,spdylay))


$(targets_phantom_module): /usr/share/phantom/module.mk

# vim: ft=make
