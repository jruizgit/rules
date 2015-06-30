OBJ=events.o json.o net.o rete.o state.o
LIBNAME=rules

# Fallback to gcc when $CC is not in $PATH.
CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
OPTIMIZATION?=-O3
WARNINGS=-Wall
DEBUG?= -g -ggdb
REAL_CFLAGS=$(OPTIMIZATION) -fPIC $(CFLAGS) $(WARNINGS) $(DEBUG) $(ARCH)
REAL_LDFLAGS=$(LDFLAGS) $(ARCH)

REAL_CFLAGS+= -I../../deps/hiredis
REAL_LDFLAGS+= ../deps/hiredis/libhiredis.a

STLIBSUFFIX=a
STLIBNAME=$(LIBNAME).$(STLIBSUFFIX)
STLIB_MAKE_CMD=ar rcs $(STLIBNAME)

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),SunOS)
  REAL_LDFLAGS+= -ldl -lnsl -lsocket
  INSTALL= cp -r
endif

all: $(STLIBNAME)

# Deps (use make dep to generate this)
events.o: events.c rules.h net.h json.h
json.o: json.c json.h rules.h
net.o: net.c net.h rules.h json.h rete.h
rete.o: rete.c rete.h net.h json.h state.h
state.o: state.c state.h json.h net.h

$(STLIBNAME): $(OBJ)
	$(STLIB_MAKE_CMD) $(OBJ)

static: $(STLIBNAME)

rules-%: %.o $(STLIBNAME)
	$(CC) -o $@ $(REAL_LDFLAGS) $< $(STLIBNAME)

.c.o:
	$(CC) -std=c99 -c $(REAL_CFLAGS) $<

clean:
	rm -rf $(STLIBNAME) $(BINS) *.o *.gcda *.gcno *.gcov

gprof:
	$(MAKE) CFLAGS="-pg" LDFLAGS="-pg"

noopt:
	$(MAKE) OPTIMIZATION=""

.PHONY: all clean gprof gcov noopt