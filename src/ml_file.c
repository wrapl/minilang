#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gc.h>
#include <errno.h>
#include <dirent.h>
#include "ml_file.h"
#include "ml_macros.h"
#include "ml_stream.h"

#undef ML_CATEGORY
#define ML_CATEGORY "file"

typedef struct ml_file_t {
	ml_type_t *Type;
	FILE *Handle;
} ml_file_t;

static void ml_file_finalize(ml_file_t *File, void *Data) {
	if (File->Handle) {
		fclose(File->Handle);
		File->Handle = NULL;
	}
}

ML_FUNCTION(MLFileOpen) {
//!file
//@file
//<Path
//<Mode
//>file
// Opens the file at :mini:`Path` depending on :mini:`Mode`,
//
// * :mini:`"r"`: opens the file for reading,
// * :mini:`"w"`: opens the file for writing,
// * :mini:`"a"`: opens the file for appending.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	const char *Mode = ml_string_value(Args[1]);
	FILE *Handle = fopen(Path, Mode);
	if (!Handle) return ml_error("FileError", "failed to open %s in mode %s: %s", Path, Mode, strerror(errno));
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

ML_TYPE(MLFileT, (MLStreamT), "file",
// A file handle for reading / writing.
	.Constructor = (ml_value_t *)MLFileOpen
);

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
		*Result = snew(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

static void ML_TYPED_FN(ml_stream_read, MLFileT, ml_state_t *Caller, ml_file_t *File, void *Address, int Count) {
	if (!File->Handle) ML_ERROR("FileError", "reading from closed file");
	ssize_t Result = fread(Address, 1, Count, File->Handle);
	if (Result < 0) ML_ERROR("FileError", "error reading from file: %s", strerror(errno));
	ML_RETURN(ml_integer(Result));
}

static void ML_TYPED_FN(ml_stream_write, MLFileT, ml_state_t *Caller, ml_file_t *File, const void *Address, int Count) {
	if (!File->Handle) ML_ERROR("FileError", "wrtiting to closed file");
	ssize_t Result = fwrite(Address, 1, Count, File->Handle);
	if (Result < 0) ML_ERROR("FileError", "error writing to file: %s", strerror(errno));
	ML_RETURN(ml_integer(Result));
}

ML_METHOD("eof", MLFileT) {
//<File
//>File | nil
// Returns :mini:`File` if :mini:`File` is closed, otherwise return :mini:`nil`.
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file already closed");
	if (feof(File->Handle)) return Args[0];
	return MLNil;
}

ML_METHOD("flush", MLFileT) {
//<File
// Flushes any pending writes to :mini:`File`.
	ml_file_t *File = (ml_file_t *)Args[0];
	if (File->Handle) fflush(File->Handle);
	return (ml_value_t *)File;
}

ML_METHOD("close", MLFileT) {
//<File
// Closes :mini:`File`.
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "File already closed");
	fclose(File->Handle);
	File->Handle = NULL;
	return MLNil;
}

ml_value_t *ml_file(FILE *Handle) {
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

ML_FUNCTION(MLFileRename) {
//!file
//@file::rename
//<Old
//<New
// Renames the file :mini:`Old` to :mini:`New`.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *OldName = ml_string_value(Args[0]);
	const char *NewName = ml_string_value(Args[1]);
	if (rename(OldName, NewName)) {
		return ml_error("FileError", "failed to rename %s to %s: %s", OldName, NewName, strerror(errno));
	}
	return MLNil;
}

ML_FUNCTION(MLFileUnlink) {
//!file
//@file::unlink
//<Path
// Removes the file at :mini:`Path`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	if (unlink(Name)) {
		return ml_error("FileError", "failed to unlink %s: %s", Name, strerror(errno));
	}
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	DIR *Handle;
	ml_value_t *Entry;
	int Index;
} ml_dir_t;

static void ml_dir_finalize(ml_dir_t *Dir, void *Data) {
	if (Dir->Handle) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
	}
}

extern ml_type_t MLDirT[];

ML_FUNCTION(MLDirOpen) {
//@dir
//<Path
//>dir
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	DIR *Handle = opendir(Path);
	if (!Handle) return ml_error("FileError", "failed to open %s: %s", Path, strerror(errno));
	ml_dir_t *Dir = new(ml_dir_t);
	Dir->Type = MLDirT;
	Dir->Handle = Handle;
	GC_register_finalizer(Dir, (void *)ml_dir_finalize, 0, 0, 0);
	return (ml_value_t *)Dir;
}

ML_TYPE(MLDirT, (MLSequenceT), "directory",
	.Constructor = (ml_value_t *)MLDirOpen
);

ML_METHOD("read", MLDirT) {
//<Dir
//>string
	ml_dir_t *Dir = (ml_dir_t *)Args[0];
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) return MLNil;
	return ml_string(GC_strdup(Entry->d_name), -1);
}

static void ML_TYPED_FN(ml_iter_key, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	ML_RETURN(ml_integer(Dir->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	ML_RETURN(Dir->Entry);
}

static void ML_TYPED_FN(ml_iter_next, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
		ML_RETURN(MLNil);
	}
	++Dir->Index;
	Dir->Entry = ml_string(GC_strdup(Entry->d_name), -1);
	ML_RETURN(Dir);
}

static void ML_TYPED_FN(ml_iterate, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
		ML_RETURN(MLNil);
	}
	Dir->Index = 1;
	Dir->Entry = ml_string(GC_strdup(Entry->d_name), -1);
	ML_RETURN(Dir);
}

static void ml_popen_finalize(ml_file_t *File, void *Data) {
	if (File->Handle) {
		pclose(File->Handle);
		File->Handle = NULL;
	}
}

extern ml_type_t MLPOpenT[];

ML_FUNCTION(MLPOpen) {
//@popen
//<Command
//<Mode
//>popen
// Executes :mini:`Command` with the shell and returns an open file to communicate with the subprocess depending on :mini:`Mode`,
//
// * :mini:`"r"`: opens the file for reading,
// * :mini:`"w"`: opens the file for writing.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *Command = ml_string_value(Args[0]);
	const char *Mode = ml_string_value(Args[1]);
	FILE *Handle = popen(Command, Mode);
	if (!Handle) return ml_error("FileError", "failed to run %s in mode %s: %s", Command, Mode, strerror(errno));
	ml_file_t *File = new(ml_file_t);
	File->Type = MLPOpenT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_popen_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

ML_TYPE(MLPOpenT, (MLFileT), "popen",
// A file that reads or writes to a running subprocess.
	.Constructor = (ml_value_t *)MLPOpen
);

ML_METHOD("close", MLPOpenT) {
//<File
//>integer
// Waits for the subprocess to finish and returns the exit status.
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "File already closed");
	int Result = pclose(File->Handle);
	File->Handle = NULL;
	return ml_integer(Result);
}

void ml_file_init(stringmap_t *Globals) {
#include "ml_file_init.c"
	stringmap_insert(MLFileT->Exports, "rename", MLFileRename);
	stringmap_insert(MLFileT->Exports, "unlink", MLFileUnlink);
	if (Globals) {
		stringmap_insert(Globals, "file", MLFileT);
		stringmap_insert(Globals, "dir", MLDirT);
		stringmap_insert(Globals, "popen", MLPOpenT);
	}
}
