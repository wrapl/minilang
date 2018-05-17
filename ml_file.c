#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gc.h>
#include <ml_file.h>

#define new(T) ((T *)GC_MALLOC(sizeof(T)))
#define anew(T, N) ((T *)GC_MALLOC((N) * sizeof(T)))
#define snew(N) ((char *)GC_MALLOC_ATOMIC(N))
#define xnew(T, N, U) ((T *)GC_MALLOC(sizeof(T) + (N) * sizeof(U)))

typedef struct ml_file_t ml_file_t;

struct ml_file_t {
	const ml_type_t *Type;
	FILE *Handle;
};

ml_type_t MLFileT[1] = {{
	MLAnyT, "file",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

static ml_value_t *ml_file_read_line(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	char *Line = 0;
	size_t Length = 0;
	ssize_t Read = getline(&Line, &Length, File->Handle);
	if (Read < 0) return feof(File->Handle) ? MLNil : ml_error("FileError", "error reading from file");
	return ml_string(Line, Read);
}

static ml_value_t *ml_file_read_count(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (feof(File->Handle)) return MLNil;
	ssize_t Requested = ml_integer_value(Args[1]);
	char *Chars = snew(Requested + 1);
	ssize_t Actual = fread(Chars, 1, Requested, File->Handle);
	if (Actual == 0) return MLNil;
	if (Actual < 0) return ml_error("FileError", "error reading from file");
	Chars[Actual] = 0;
	return ml_string(Chars, Actual);
}

static ml_value_t *ml_file_write_string(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	const char *Chars = ml_string_value(Args[1]);
	ssize_t Remaining = ml_string_length(Args[1]);
	while (Remaining > 0) {
		ssize_t Actual = fwrite(Chars, 1, Remaining, File->Handle);
		if (Actual < 0) return ml_error("FileError", "error writing to file");
		Chars += Actual;
		Remaining -= Actual;
	}
	return Args[0];
}

static int ml_file_write_buffer_chars(const char *Chars, size_t Remaining, ml_file_t *File) {
	while (Remaining > 0) {
		ssize_t Actual = fwrite(Chars, 1, Remaining, File->Handle);
		if (Actual < 0) return 1;
		Chars += Actual;
		Remaining -= Actual;
	}
	return 0;
}

static ml_value_t *ml_file_write_buffer(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[1];
	if (ml_stringbuffer_foreach(Buffer, File, (void *)ml_file_write_buffer_chars)) return ml_error("FileError", "error writing to file");
	return Args[0];
}

static ml_value_t *ml_file_eof(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (feof(File->Handle)) return Args[0];
	return MLNil;
}

static ml_value_t *ml_file_close(void *Data, int Count, ml_value_t **Args) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (File->Handle) {
		fclose(File->Handle);
		File->Handle = 0;
	}
	return MLNil;
}

static void ml_file_finalize(ml_file_t *File, void *Data) {
	if (File->Handle) {
		fclose(File->Handle);
		File->Handle = 0;
	}
}

ml_value_t *ml_file_new(FILE *Handle) {
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

ml_value_t *ml_file_open(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	const char *Mode = ml_string_value(Args[1]);
	FILE *Handle = fopen(Path, Mode);
	if (!Handle) return ml_error("FileError", "failed to open %s in mode %s", Path, Mode);
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

void ml_file_init() {
	ml_method_by_name("read", 0, ml_file_read_line, MLFileT, NULL);
	ml_method_by_name("read", 0, ml_file_read_count, MLFileT, MLIntegerT, NULL);
	ml_method_by_name("write", 0, ml_file_write_string, MLFileT, MLStringT, NULL);
	ml_method_by_name("write", 0, ml_file_write_buffer, MLFileT, MLStringBufferT, NULL);
	ml_method_by_name("eof", 0, ml_file_eof, MLFileT, NULL);
	ml_method_by_name("close", 0, ml_file_close, MLFileT, NULL);
}
