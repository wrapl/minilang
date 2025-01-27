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

obj/%.o: src/%.c obj/ml_config.h | obj
	$(CC) $(CFLAGS) -c -o $@ $< 

obj/%_init.c: src/%.c | obj src/*.h 
	cc -E -P -DGENERATE_INIT $(CFLAGS) $< | sed -f sed.txt | grep -o 'INIT_CODE .*);' | sed 's/INIT_CODE //g' > $@

obj/ml_bytecode.o: obj/ml_bytecode_init.c src/*.h
obj/ml_cbor.o: obj/ml_cbor_init.c src/*.h
obj/ml_compiler.o: obj/ml_compiler_init.c src/*.h
obj/ml_console.o: obj/ml_console_init.c src/*.h
obj/ml_file.o: obj/ml_file_init.c src/*.h
obj/ml_json.o: obj/ml_json_init.c src/*.h
obj/ml_list.o: obj/ml_list_init.c src/*.h
obj/ml_logging.o: obj/ml_logging_init.c src/*.h
obj/ml_map.o: obj/ml_map_init.c src/*.h
obj/ml_math.o: obj/ml_math_init.c src/*.h
obj/ml_method.o: obj/ml_method_init.c src/*.h
obj/ml_number.o: obj/ml_number_init.c src/*.h
obj/ml_object.o: obj/ml_object_init.c src/*.h
obj/ml_runtime.o: obj/ml_runtime_init.c src/*.h
obj/ml_sequence.o: obj/ml_sequence_init.c src/*.h
obj/ml_set.o: obj/ml_set_init.c src/*.h
obj/ml_slice.o: obj/ml_slice_init.c src/*.h
obj/ml_socket.o: obj/ml_socket_init.c src/*.h
obj/ml_stream.o: obj/ml_stream_init.c src/*.h
obj/ml_string.o: obj/ml_string_init.c src/*.h
obj/ml_time.o: obj/ml_time_init.c src/*.h
obj/ml_types.o: obj/ml_types_init.c src/*.h

common_objects = \
	obj/inthash.o \
	obj/ml_bytecode.o \
	obj/ml_cbor.o \
	obj/ml_compiler.o \
	obj/ml_console.o \
	obj/ml_debugger.o \
	obj/ml_file.o \
	obj/ml_json.o \
	obj/ml_list.o \
	obj/ml_logging.o \
	obj/ml_map.o \
	obj/ml_method.o \
	obj/ml_number.o \
	obj/ml_object.o \
	obj/ml_opcodes.o \
	obj/ml_runtime.o \
	obj/ml_sequence.o \
	obj/ml_set.o \
	obj/ml_slice.o \
	obj/ml_socket.o \
	obj/ml_stream.o \
	obj/ml_string.o \
	obj/ml_time.o \
	obj/ml_types.o \
	obj/sha256.o \
	obj/stringmap.o

platform_objects =

ifeq ($(MACHINE), i686)
	override CFLAGS += -fno-pic
endif

ifeq ($(PLATFORM), Linux)
	platform_objects += obj/linenoise.o
	override CFLAGS += -DUSE_LINENOISE
	override LDFLAGS += -lgc
endif

ifeq ($(PLATFORM), Android)
	platform_objects += obj/linenoise.o
	override LDFLAGS += -lgc
endif

ifeq ($(PLATFORM), FreeBSD)
	platform_objects += obj/linenoise.o
	override CFLAGS += -I/usr/local/include
	override LDFLAGS += -L/usr/local/lib -lgc-threaded
endif

ifeq ($(PLATFORM), Darwin)
	platform_objects += obj/linenoise.o
	override LDFLAGS += -lgc
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

install_h = \
	$(install_include)/linenoise.h \
	$(install_include)/minilang.h \
	$(install_include)/ml_bytecode.h \
	$(install_include)/ml_cbor.h \
	$(install_include)/ml_compiler.h \
	$(install_include)/ml_console.h \
	$(install_include)/ml_file.h \
	$(install_include)/ml_json.h \
	$(install_include)/ml_macros.h \
	$(install_include)/ml_object.h \
	$(install_include)/ml_opcodes.h \
	$(install_include)/ml_runtime.h \
	$(install_include)/ml_sequence.h \
	$(install_include)/ml_time.h \
	$(install_include)/ml_types.h \
	$(install_include)/sha256.h \
	$(install_include)/stringmap.h

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

	