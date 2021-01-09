CC=g++
CXX=$(CC)
CFLAGS=-std=c++11 -g -Wall -pthread -I./ -I./rocksdb/include/
CXXFLAGS=$(CFLAGS)
ROCKSDB_LIB_DIR=$(CURDIR)/rocksdb/lib/
LDFLAGS= -lpthread -ltbb -lhiredis -Wl,-rpath,$(ROCKSDB_LIB_DIR) -L$(ROCKSDB_LIB_DIR) -lrocksdb
SUBDIRS=redis rocksdb
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

init-rocksdb:
	$(MAKE) -C rocksdb init-headers

$(OBJECTS): init-rocksdb

$(SUBDIRS): $(OBJECTS)
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS) $(SUBDIRS)
	$(CC) $(CFLAGS) $(wildcard *.cc) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC) init-rocksdb
