#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gc.h>
#include "ml_file.h"
#include "ml_macros.h"

#define new(T) ((T *)GC_MALLOC(sizeof(T)))
#define anew(T, N) ((T *)GC_MALLOC((N) * sizeof(T)))
#define snew(N) ((char *)GC_MALLOC_ATOMIC(N))
#define xnew(T, N, U) ((T *)GC_MALLOC(sizeof(T) + (N) * sizeof(U)))

typedef struct ml_file_t {
	const ml_type_t *Type;
	FILE *Handle;
} ml_file_t;

ml_type_t *MLFileT;

FILE *ml_file_handle(ml_value_t *Value) {
	return ((ml_file_t *)Value)->Handle;
}

#ifdef __MINGW32__
static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = GC_MALLOC_ATOMIC(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

ML_METHOD("read", MLFileT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	char *Line = 0;
	size_t Length = 0;
#ifdef __MINGW32__
	ssize_t Read = ml_read_line(File->Handle, 0, &Line);
#else
	ssize_t Read = getline(&Line, &Length, File->Handle);
#endif
	if (Read < 0) return feof(File->Handle) ? MLNil : ml_error("FileError", "error reading from file");
	return ml_string(Line, Read);
}

ML_METHOD("read", MLFileT, MLIntegerT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (feof(File->Handle)) return MLNil;
	ssize_t Requested = ml_integer_value(Args[1]);
	ml_stringbuffer_t Final[1] = {ML_STRINGBUFFER_INIT};
	char Buffer[ML_STRINGBUFFER_NODE_SIZE];
	while (Requested >= ML_STRINGBUFFER_NODE_SIZE) {
		ssize_t Actual = fread(Buffer, 1, ML_STRINGBUFFER_NODE_SIZE, File->Handle);
		if (Actual < 0) return ml_error("FileError", "error reading from file");
		if (Actual == 0) return ml_stringbuffer_get_string(Final);
		ml_stringbuffer_add(Final, Buffer, Actual);
		Requested -= Actual;
	}
	while (Requested > 0) {
		ssize_t Actual = fread(Buffer, 1, Requested, File->Handle);
		if (Actual < 0) return ml_error("FileError", "error reading from file");
		if (Actual == 0) return ml_stringbuffer_get_string(Final);
		ml_stringbuffer_add(Final, Buffer, Actual);
		Requested -= Actual;
	}
	return ml_stringbuffer_get_string(Final);
}

ML_METHOD("write", MLFileT, MLStringT) {
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

ML_METHOD("write", MLFileT, MLStringBufferT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[1];
	if (ml_stringbuffer_foreach(Buffer, File, (void *)ml_file_write_buffer_chars)) return ml_error("FileError", "error writing to file");
	return Args[0];
}

ML_METHOD("eof", MLFileT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (feof(File->Handle)) return Args[0];
	return MLNil;
}

ML_METHOD("close", MLFileT) {
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

void ml_file_init(stringmap_t *Globals) {
	MLFileT = ml_type(MLAnyT, "file");
	if (Globals) {
		stringmap_insert(Globals, "open", ml_function(0, ml_file_open));
	}
#include "ml_file_init.c"
}