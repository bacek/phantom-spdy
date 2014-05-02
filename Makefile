default: all


include phantom/Makefile.inc

# ===

ifeq ($(OPT),0)
CPPDEFS += -D DEBUG_ALLOC
endif

FIXINC = -isystem . -isystem ./pd/fixinclude

include /usr/share/phantom/opts.mk
others_pd += /usr/share/phantom/opts.mk

# ===

-include debian/Makefile.inc

# ===

all: deps lib/phantom $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

deps:
	mkdir deps

lib:
	mkdir lib

lib/phantom: lib
	mkdir lib/phantom

.PHONY: default all clean
