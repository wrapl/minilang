.PHONY: clean all install

PLATFORM = $(shell uname)

all: minilang minipp libminilang.a

*.o: *.h

CFLAGS += -std=gnu99 -fstrict-aliasing -Wstrict-aliasing -I. -pthread -DGC_THREADS -D_GNU_SOURCE
LDFLAGS += -lm -lgc

ifdef DEBUG
	CFLAGS += -g -DGC_DEBUG
	LDFLAGS += -g
else
	CFLAGS += -O2
endif

common_objects = \
	minilang.o \
	ml_compiler.o \
	ml_runtime.o \
	ml_types.o \
	ml_file.o \
	sha256.o \
	stringmap.o \
	ml_console.o

platform_objects =

ifeq ($(PLATFORM), Linux)
	platform_objects += linenoise.o 
endif

minilang_objects = $(common_objects) $(platform_objects) \
	ml.o

minilang: Makefile $(minilang_objects) *.h
	gcc $(minilang_objects) $(LDFLAGS) -o$@

minipp_objects = $(common_objects) $(platform_objects) \
	minipp.o

minipp: Makefile $(minipp_objects) *.h
	gcc $(minipp_objects) $(LDFLAGS) -o$@

libminilang.a: $(common_objects) $(platform_objects)
	ar rcs $@ $(common_objects) $(platform_objects)

clean:
	rm -f minilang
	rm -f minipp
	rm -f *.o
	rm -f libminilang.a

PREFIX = /usr
install_include = $(PREFIX)/include/minilang
install_lib = $(PREFIX)/lib

install_h = \
	$(install_include)/linenoise.h \
	$(install_include)/minilang.h \
	$(install_include)/ml_console.h \
	$(install_include)/ml_file.h \
	$(install_include)/ml_macros.h \
	$(install_include)/ml_types.h \
	$(install_include)/sha256.h \
	$(install_include)/stringmap.h

install_a = $(install_lib)/libminilang.a

$(install_h): $(install_include)/%: %
	mkdir -p $(install_include)
	cp $< $@

$(install_a): $(install_lib)/%: %
	mkdir -p $(install_lib)
	cp $< $@

install: $(install_h) $(install_a)
