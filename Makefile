###
# Install to ./run/gphome
# datadir set to ./run/data
# 
# Typical usage
# % make config         ## [ or config-opt for optimized build ]
# % make clean 
# % make run 		## this will install, stop, start
#
SHELL=/bin/bash
DESTDIR := $(shell touch INSTALLDIR && cat INSTALLDIR)
ifeq ($(DESTDIR),)
	DESTDIR := $(CURDIR)/run/gphome
endif
ifneq (${prefix},)
	DESTDIR := ${prefix}
endif

EXX_ENGINE=gpdb/src/include/exx/exx_engine.h

all: 
ifndef GOPATH
	$(error GOPATH is undefined)
endif
	@cd toolchain || (echo 'ERROR: please symlink toolchain' && exit 1)
	@cd madlib || (echo 'ERROR: please symlink madlib' && exit 1)
	$(MAKE) -C gpdb
	$(MAKE) -C gpdb/contrib
	cd dg/src/vitessedata/dg && go get . && go install

install: all
	@rm -rf ./run/gphome
	$(MAKE) -C gpdb install
	$(MAKE) -C gpdb/contrib install
	( (cd toolchain/installed/lib && tar cf - libxml*.so* ) | \
		(cd $(DESTDIR)/lib && tar xf - ) )
	( (cd toolchain/installed/lib && tar cf - libzstd*.so* ) | \
		(cd $(DESTDIR)/lib && tar xf - ) )
	# gpopt
	( (cd /usr/local/lib && tar cf - libgp* libnaucrates* libxerces*) | \
		(cd $(DESTDIR)/lib && tar xf - ) ) 
	# gpfdist
	(cd gpdb/gpAux/extensions/gpfdist && bash build.sh && cp gpfdist $(DESTDIR)/bin)
	# gpnetbench
	(cd gpdb/gpAux/platform/gpnetbench && BLD_ARCH=x86_64 make && cp gpnetbenchServer gpnetbenchClient $(DESTDIR)/bin/lib)
	# dg
	install dg/bin/* $(DESTDIR)/bin

runbg: install
	-(source run/env.sh && gpstop -a -M fast)
	-pkill postgres 2> /dev/null 
	find run -name 'gpdb-*.csv' -exec rm {} \;
	-(source run/env.sh && gpstart -a)

run: runbg

run4: install
	-(source run/env4.sh && gpstop -a -M fast)
	-pkill postgres 2> /dev/null 
	find run -name 'gpdb-*.csv' -exec rm {} \;
	-(source run/env4.sh && gpstart -a)

clean:
	$(MAKE) -C gpdb clean
	$(MAKE) -C gpdb/gpMgmt clean
	rm -f dg/bin/*
	cd dg/src/vitessedata/dg && go clean -i 
	rm -f $(EXX_ENGINE)

setup_dev:
	$(eval prefix ?= $(CURDIR)/run/gphome)
	$(eval DESTDIR := $(prefix) )
	echo $(DESTDIR) > INSTALLDIR
	echo '-g -O0 -Wall -Wextra -std=c99 -mavx ' > CFLAGS
	echo '-g -O0 -Wall -std=c++11 -mavx ' > CXXFLAGS

setup_opt:
	$(eval prefix ?= $(CURDIR)/run/gphome)
	$(eval DESTDIR := $(prefix) )
	echo $(DESTDIR) > INSTALLDIR
	echo '-g -O3 -DNDEBUG -Wall -Wextra -std=c99 -mavx ' > CFLAGS
	echo '-g -O3 -DNDEBUG -Wall -std=c++11 -mavx ' > CXXFLAGS

config: setup_dev
	(cd gpdb && ./configure --prefix=$(DESTDIR) --with-openssl --with-python --with-libxml --enable-debug --enable-cassert --enable-orca --enable-snmp CFLAGS='-O0 -fno-inline -I$(CURDIR)/toolchain/installed/include')

config-opt: setup_opt
	(cd gpdb && ./configure --prefix=$(DESTDIR) --with-openssl --with-python --with-libxml --enable-debug --enable-orca --enable-snmp CFLAGS='-O3 -I$(CURDIR)/toolchain/installed/include')

