.PHONY: clean all install

PLATFORM = $(shell uname)
MACHINE = $(shell uname -m)

all: bin/minilang bin/minipp lib/libminilang.a

SUBDIRS = obj bin lib

$(SUBDIRS):
	mkdir -p $@

*.o: *.h

override CFLAGS += \
	-std=gnu99 -fstrict-aliasing -foptimize-sibling-calls \
	-Wstrict-aliasing -Wall \
	-Iobj -Isrc -pthread -DGC_THREADS -D_GNU_SOURCE -D$(PLATFORM)
override LDFLAGS += -lm

ifdef DEBUG
	override CFLAGS += -g -DGC_DEBUG -DDEBUG
	override LDFLAGS += -g
else
	override CFLAGS += -O3 -g
	override LDFLAGS += -g
endif

obj/ml_config.h: | obj
	@echo "#ifndef ML_CONFIG_H" > $@ 
	@echo "#define ML_CONFIG_H" >> $@
	@echo "#endif" >> $@

obj/%.o: src/%.c | obj obj/%_init.c src/*.h obj/ml_config.h 
	$(CC) $(CFLAGS) -c -o $@ $< 

#.PRECIOUS: obj/%_init.c

obj/%_init.c: src/%.c | obj src/*.h obj/ml_config.h 
	cc -E -P -DGENERATE_INIT $(CFLAGS) $< | sed -f sed.txt | grep -o 'INIT_CODE .*);' | sed 's/INIT_CODE //g' > $@

sources = \
	boolean \
	bytecode \
	cbor \
	compiler \
	console \
	debugger \
	file \
	function \
	json \
	list \
	logging \
	map \
	method \
	number \
	object \
	opcodes \
	runtime \
	sequence \
	set \
	slice \
	socket \
	stream \
	string \
	time \
	tuple \
	types \
	uuid

common_objects = \
	$(foreach name, $(sources), obj/ml_$(name).o) \
	obj/inthash.o \
	obj/sha256.o \
	obj/stringmap.o \
	obj/uuidmap.o

platform_objects =

ifeq ($(MACHINE), i686)
	override CFLAGS += -fno-pic
endif

ifeq ($(PLATFORM), Linux)
	platform_objects += obj/linenoise.o
	override CFLAGS += -DUSE_LINENOISE
	override LDFLAGS += -lgc -luuid
endif

ifeq ($(PLATFORM), Android)
	platform_objects += obj/linenoise.o
	override LDFLAGS += -lgc -luuid
endif

ifeq ($(PLATFORM), FreeBSD)
	platform_objects += obj/linenoise.o
	override CFLAGS += -I/usr/local/include
	override LDFLAGS += -L/usr/local/lib -lgc-threaded -luuid
endif

ifeq ($(PLATFORM), Darwin)
	platform_objects += obj/linenoise.o
	override CFLAGS += -Wno-initializer-overrides
	override LDFLAGS += -lgc -luuid
endif

minilang_objects = $(common_objects) $(platform_objects) \
	obj/minilang.o

bin/minilang: bin Makefile $(minilang_objects) src/*.h
	$(CC) $(minilang_objects) $(LDFLAGS) -o$@

minipp_objects = $(common_objects) $(platform_objects) \
	obj/minipp.o

bin/minipp: bin Makefile $(minipp_objects) src/*.h
	$(CC) $(minipp_objects) $(LDFLAGS) -o$@

lib/libminilang.a: lib $(common_objects) $(platform_objects)
	ar rcs $@ $(common_objects) $(platform_objects)

clean:
	rm -rf bin lib obj

PREFIX = /usr
install_bin = $(DESTDIR)$(PREFIX)/bin
install_include = $(DESTDIR)$(PREFIX)/include/minilang
install_lib = $(DESTDIR)$(PREFIX)/lib

install_exe = \
	$(install_bin)/minilang

headers = \
	linenoise.h \
	minilang.h \
	ml_bytecode.h \
	ml_cbor.h \
	ml_compiler.h \
	ml_console.h \
	ml_file.h \
	ml_json.h \
	ml_macros.h \
	ml_object.h \
	ml_opcodes.h \
	ml_runtime.h \
	ml_sequence.h \
	ml_time.h \
	ml_types.h \
	sha256.h \
	stringmap.h \
	uuidmap.h

install_h = $(foreach header, $(headers), $(install_include)/$(header))

install_a = $(install_lib)/libminilang.a

$(install_exe): $(install_bin)/%: bin/%
	mkdir -p $(install_bin)
	cp $< $@

$(install_h): $(install_include)/%: src/%
	mkdir -p $(install_include)
	cp $< $@

$(install_a): $(install_lib)/%: lib/%
	mkdir -p $(install_lib)
	cp $< $@

install: $(install_exe) $(install_h) $(install_a)

	