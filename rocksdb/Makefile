all: init-submodule
	OPTIMIZE_LEVEL=-O3 DESTDIR=$(CURDIR) PREFIX= $(MAKE) -C src install-shared

init-submodule:
	git submodule update --init

init-headers: init-submodule
	DESTDIR=$(CURDIR) PREFIX= $(MAKE) -C src install-headers

clean:
	DESTDIR=$(CURDIR) PREFIX= $(MAKE) -C src uninstall
	$(MAKE) -C src $@

.PHONY: all clean init-submodule init-headers
