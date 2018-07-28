.PHONY: clean all

all: minilang minipp

sources = \
	sha256.c \
	minilang.c \
	ml_compiler.c \
	ml_runtime.c \
	ml_types.c \
	ml_file.c \
	stringmap.c

CFLAGS += -std=gnu99 -I. -Igc/include -g -pthread -DGC_THREADS -D_GNU_SOURCE
LDFLAGS += -lm -ldl -lgc

ifdef DEBUG
	CFLAGS += -g -DGC_DEBUG
	LDFLAGS += -g
else
	CFLAGS += -O2
endif

minilang: Makefile $(sources) *.h
	gcc $(CFLAGS) $(sources) ml.c ml_console.c linenoise.c $(LDFLAGS) -o$@

minipp: Makefile $(sources) minipp.c *.h
	gcc $(CFLAGS) $(sources) minipp.c $(LDFLAGS) -o$@

clean:
	rm minilang
	rm minipp
