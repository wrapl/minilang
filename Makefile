.PHONY: clean all

all: minilang minipp

sources = \
	sha256.c \
	minilang.c \
	ml_file.c \
	stringmap.c \
	linenoise.c \
	ml.c

CFLAGS += -std=gnu99 -I. -Igc/include -g -pthread -DGC_THREADS -D_GNU_SOURCE -DGC_DEBUG
LDFLAGS += -lm -ldl -g -lgc

minilang: Makefile $(sources) *.h
	gcc $(CFLAGS) $(sources) $(LDFLAGS) -o$@

minipp: Makefile $(sources) minipp.c *.h
	gcc $(CFLAGS) sha256.c minipp.c ml_file.c stringmap.c linenoise.c $(LDFLAGS) -o$@

clean:
	rm minilang
