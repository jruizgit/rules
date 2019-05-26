OBJ=events.o json.o rete.o state.o regex.o
LIBNAME=librules

# Fallback to gcc when $CC is not in $PATH.
CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
OPTIMIZATION?=-O3
WARNINGS=-Wall
DEBUG?= -g -ggdb
REAL_CFLAGS=$(OPTIMIZATION) -fPIC $(CFLAGS) $(WARNINGS) $(DEBUG) $(ARCH) -D_GNU_SOURCE

STLIBSUFFIX=a
STLIBNAME=$(LIBNAME).$(STLIBSUFFIX)
STLIB_MAKE_CMD=ld $(ARCH) -o $(STLIBNAME) -r
#STLIB_MAKE_CMD=ar rcs $(STLIBNAME)

all: $(STLIBNAME)

# Deps (use make dep to generate this)
events.o: events.c rules.h json.h regex.h
json.o: json.c json.h rules.h
rete.o: rete.c rete.h json.h state.h regex.h
state.o: state.c state.h json.h
regex.o: regex.c regex.h rules.h

$(STLIBNAME): $(OBJ)
	$(STLIB_MAKE_CMD) $(OBJ)

static: $(STLIBNAME)

.c.o:
	$(CC) -std=c99 -c $(REAL_CFLAGS) $<

clean:
	rm -rf $(STLIBNAME) $(BINS) *.o 

.PHONY: all clean