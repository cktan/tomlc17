.NOTPARALLEL:

prefix ?= /usr/local
override prefix := $(prefix:%/=%)  # remove trailing /
#DIRS = src tests
DIRS = src simple

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
FORMATDIRS = $(DIRS:%=format-%)

###################################

# Define PCFILE content based on prefix
define PCFILE
Name: libtomlc17
URL: https://github.com/cktan/tomlc17/
Description: TOML C library in c17.
Version: v1.0
Libs: -L${prefix}/lib -ltomlc17
Cflags: -I${prefix}/include
endef

# Make it available to subshells
export PCFILE

#################################

all: $(BUILDDIRS)

$(DIRS): $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: all
	install -d ${prefix}/include
	install -d ${prefix}/lib
	install -d ${prefix}/lib/pkgconfig
	install -m 0644 -t ${prefix}/include src/tomlc17.h
	install -m 0644 -t ${prefix}/lib src/libtomlc17.a
	@echo "$$PCFILE" >> ${prefix}/lib/pkgconfig/tomlc17.pc

format: $(FORMATDIRS)

clean: $(CLEANDIRS)

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

$(FORMATDIRS):
	$(MAKE) -C $(@:format-%=%) format

.PHONY: $(DIRS) $(BUILDDIRS) $(CLEANDIRS) $(FORMATDIRS)
.PHONY: all install format
