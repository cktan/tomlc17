.NOTPARALLEL:

prefix ?= /usr/local
# remove trailing /
override prefix := $(prefix:%/=%)
DIRS := src simple test

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
FORMATDIRS = $(DIRS:%=format-%)
TESTDIRS = $(DIRS:%=test-%)

###################################

# Define PCFILE content based on prefix
define PCFILE
prefix=${prefix}
Name: libtomlc17
URL: https://github.com/cktan/tomlc17/
Description: TOML C library in c17.
Version: v1.0
Libs: -L$${prefix}/lib -ltomlc17
Cflags: -I$${prefix}/include
endef

# Make it available to subshells
export PCFILE

#################################

all: $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: all
	install -d $(DESTDIR)${prefix}/include
	install -d $(DESTDIR)${prefix}/lib
	install -d $(DESTDIR)${prefix}/lib/pkgconfig
	install -m 0644 src/tomlc17.h $(DESTDIR)${prefix}/include/
	install -m 0644 src/tomlcpp.hpp $(DESTDIR)${prefix}/include/
	install -m 0644 src/libtomlc17.a $(DESTDIR)${prefix}/lib/
	@echo "$$PCFILE" >> $(DESTDIR)${prefix}/lib/pkgconfig/libtomlc17.pc

test: $(TESTDIRS)

format: $(FORMATDIRS)

clean: $(CLEANDIRS)

$(TESTDIRS):
	$(MAKE) -C $(@:test-%=%) test

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

$(FORMATDIRS):
	$(MAKE) -C $(@:format-%=%) format

.PHONY: $(DIRS) $(BUILDDIRS) $(TESTDIRS) $(CLEANDIRS) $(FORMATDIRS)
.PHONY: all install tet format clean
