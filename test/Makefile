.NOTPARALLEL:

# disable merge tests for now
DIRS = scankey scanvalue parser merge stdtest

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
FORMATDIRS = $(DIRS:%=format-%)
TESTDIRS = $(DIRS:%=test-%)


all: $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)


test: $(TESTDIRS)

format: $(FORMATDIRS)

clean: $(CLEANDIRS)

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

$(FORMATDIRS):
	$(MAKE) -C $(@:format-%=%) format

$(TESTDIRS):
	$(MAKE) -C $(@:test-%=%) test

.PHONY: $(DIRS) $(BUILDDIRS) $(CLEANDIRS) $(FORMATDIRS)
.PHONY: all install format test

