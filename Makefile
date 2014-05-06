default: all

include phantom/Makefile.inc

# ===

ifeq ($(OPT),0)
CPPDEFS += -D DEBUG_ALLOC
endif

FIXINC = -isystem . -isystem ./pd/fixinclude -isystem ../phantom

include /usr/share/phantom/opts.mk
others_pd += /usr/share/phantom/opts.mk

# ===

-include debian/Makefile.inc

# ===

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

$(targets): deps lib/phantom

.PHONY: default all clean

%.o: deps

%.s.o: deps

