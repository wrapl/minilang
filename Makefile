.PHONY: clean all install

PLATFORM = $(shell uname)
MACHINE = $(shell uname -m)

all: minilang minipp libminilang.a

*.o: *.h

CFLAGS += -std=gnu99 -fstrict-aliasing -Wstrict-aliasing -Wall \
	-I. -pthread -DGC_THREADS -D_GNU_SOURCE
LDFLAGS += -lm

ifdef DEBUG
	CFLAGS += -g -DGC_DEBUG -DDEBUG
	LDFLAGS += -g
else
	CFLAGS += -O3 -g
	LDFLAGS += -g
endif

%_init.c: %.c
	echo "" > $@
	cc -E -Xpreprocessor -ftrack-macro-expansion=0 -DGENERATE_INIT $< | grep -o 'ml_[a-z]*_by_name([^{]*_fn_[^{]*);' > $@

ml_types.o: ml_types_init.c
ml_iterfns.o: ml_iterfns_init.c

common_objects = \
	minilang.o \
	ml_compiler.o \
	ml_runtime.o \
	ml_types.o \
	ml_file.o \
	ml_iterfns.o \
	sha256.o \
	stringmap.o \
	ml_console.o \
	ml_object.o

platform_objects =

ifeq ($(MACHINE), i686)
	CFLAGS += -fno-pic
endif

ifeq ($(PLATFORM), Linux)
	platform_objects += linenoise.o
	LDFLAGS += -lgc
endif

ifeq ($(PLATFORM), FreeBSD)
	platform_objects += linenoise.o
	CFLAGS += -I/usr/local/include
	LDFLAGS += -L/usr/local/lib -lgc-threaded
endif

ifeq ($(PLATFORM), Darwin)
	platform_objects += linenoise.o
	LDFLAGS += -lgc
endif

minilang_objects = $(common_objects) $(platform_objects) \
	ml_main.o

minilang: Makefile $(minilang_objects) *.h
	$(CC) $(minilang_objects) $(LDFLAGS) -o$@

minipp_objects = $(common_objects) $(platform_objects) \
	minipp.o

minipp: Makefile $(minipp_objects) *.h
	$(CC) $(minipp_objects) $(LDFLAGS) -o$@

libminilang.a: $(common_objects) $(platform_objects)
	ar rcs $@ $(common_objects) $(platform_objects)

clean:
	rm -f minilang
	rm -f minipp
	rm -f *.o
	rm -rf *_init.c
	rm -f libminilang.a

PREFIX = /usr
install_include = $(DESTDIR)$(PREFIX)/include/minilang
install_lib = $(DESTDIR)$(PREFIX)/lib

install_h = \
	$(install_include)/linenoise.h \
	$(install_include)/minilang.h \
	$(install_include)/ml_console.h \
	$(install_include)/ml_file.h \
	$(install_include)/ml_iterfns.h \
	$(install_include)/ml_macros.h \
	$(install_include)/ml_types.h \
	$(install_include)/ml_object.h \
	$(install_include)/ml_compiler.h \
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
