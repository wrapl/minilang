#include "../ml_library.h"
#include "../ml_macros.h"
#include "../ml_object.h"
#include <sqlite3.h>
#include <string.h>

typedef struct {
	ml_type_t *Type;
	sqlite3 *Handle;
} ml_sqlite_t;

static ML_FLAGS(MLSqliteOpenFlags, "sqlite-open-flags",
	"READONLY",
	"READWRITE",
	"CREATE",
	"DELETEONCLOSE",
	"EXCLUSIVE",
	"AUTOPROXY",
	"URI",
	"MEMORY",
	"MAIN_DB",
	"TEMP_DB",
	"TRANSIENT_DB",
	"MAIN_JOURNAL",
	"TEMP_JOURNAL",
	"SUBJOURNAL",
	"SUPER_JOURNAL",
	"NOMUTEX",
	"FULLMUTEX",
	"SHAREDCACHE",
	"PRIVATECACHE",
	"WAL"
);

extern ml_type_t MLSqliteT[];

static void ml_sqlite_finalize(ml_sqlite_t *Sqlite, void *Data) {
	if (Sqlite->Handle) {
		sqlite3_close_v2(Sqlite->Handle);
		Sqlite->Handle = NULL;
	}
}

ML_FUNCTION(MLSqlite) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLSqliteOpenFlags);
	const char *FileName = ml_string_value(Args[0]);
	int Flags = ml_flags_value(Args[1]);
	const char *VFS = NULL;
	if (Count > 2) {
		ML_CHECK_ARG_TYPE(2, MLStringT);
		VFS = ml_string_value(Args[2]);
	}
	ml_sqlite_t *Sqlite = new(ml_sqlite_t);
	Sqlite->Type = MLSqliteT;
	int Status = sqlite3_open_v2(FileName, &Sqlite->Handle, Flags, VFS);
	if (Status != SQLITE_OK) {
		ml_value_t *Error = ml_error("SqliteError", "Error opening database: %s", sqlite3_errmsg(Sqlite->Handle));
		sqlite3_close_v2(Sqlite->Handle);
		return Error;
	}
	GC_register_finalizer(Sqlite, (void *)ml_sqlite_finalize, NULL, NULL, NULL);
	return (ml_value_t *)Sqlite;
}

ML_TYPE(MLSqliteT, (), "sqlite",
	.Constructor = (ml_value_t *)MLSqlite
);

ML_METHOD("close", MLSqliteT) {
	ml_sqlite_t *Sqlite = (ml_sqlite_t *)Args[0];
	if (!Sqlite->Handle) return ml_error("SqliteError", "Connection already closed");
	sqlite3_close_v2(Sqlite->Handle);
	Sqlite->Handle = NULL;
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	sqlite3_stmt *Handle;
	sqlite3 *Sqlite;
	int Index, Status;
} ml_sqlite_stmt_t;

static void ml_sqlite_stmt_call(ml_state_t *Caller, ml_sqlite_stmt_t *Stat, int Count, ml_value_t **Args) {
	sqlite3_reset(Stat->Handle);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I];
		int Status;
		if (Arg == MLNil) {
			Status = sqlite3_bind_null(Stat->Handle, I + 1);
		} else if (ml_is(Arg, MLIntegerT)) {
			Status = sqlite3_bind_int64(Stat->Handle, I + 1, ml_integer_value(Arg));
		} else if (ml_is(Arg, MLBooleanT)) {
			Status = sqlite3_bind_int(Stat->Handle, I + 1, ml_boolean_value(Arg));
		} else if (ml_is(Arg, MLRealT)) {
			Status = sqlite3_bind_double(Stat->Handle, I + 1, ml_real_value(Arg));
		} else if (ml_is(Arg, MLStringT)) {
			Status = sqlite3_bind_text(Stat->Handle, I + 1, ml_string_value(Arg), ml_string_length(Arg), SQLITE_STATIC);
		} else if (ml_is(Arg, MLIntegerT)) {
			Status = sqlite3_bind_int64(Stat->Handle, I + 1, ml_integer_value(Arg));
		} else {
			ML_ERROR("SqliteError", "Unsupported parameter type %s", ml_typeof(Arg)->Name);
		}
		if (Status != SQLITE_OK) {
			ML_ERROR("SqliteError", "Error binding parameter: %s", sqlite3_errmsg(Stat->Sqlite));
		}
	}
	Stat->Status = sqlite3_step(Stat->Handle);
	if (Stat->Status == SQLITE_ERROR) {
		ML_ERROR("SqliteError", "Error executing statement: %s", sqlite3_errmsg(Stat->Sqlite));
	}
	Stat->Index = 1;
	ML_RETURN(Stat);
}

ML_TYPE(MLSqliteStmtT, (MLIteratableT), "sqlite-statement",
	.call = (void *)ml_sqlite_stmt_call
);

static void ml_sqlite_stat_finalize(ml_sqlite_stmt_t *Stat, void *Data) {
	sqlite3_finalize(Stat->Handle);
}

static void ML_TYPED_FN(ml_iterate, MLSqliteStmtT, ml_state_t *Caller, ml_sqlite_stmt_t *Stat) {
	if (Stat->Status == SQLITE_ROW) ML_RETURN(Stat);
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_next, MLSqliteStmtT, ml_state_t *Caller, ml_sqlite_stmt_t *Stat) {
	Stat->Status = sqlite3_step(Stat->Handle);
	if (Stat->Status == SQLITE_ERROR) {
		ML_ERROR("SqliteError", "Error executing statement: %s", sqlite3_errmsg(Stat->Sqlite));
	} else if (Stat->Status == SQLITE_ROW) {
		++Stat->Index;
		ML_RETURN(Stat);
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLSqliteStmtT, ml_state_t *Caller, ml_sqlite_stmt_t *Stat) {
	ML_RETURN(ml_integer(Stat->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLSqliteStmtT, ml_state_t *Caller, ml_sqlite_stmt_t *Stat) {
	int Count = sqlite3_column_count(Stat->Handle);
	ml_value_t *Tuple = ml_tuple(Count);
	for (int I = 0; I < Count; ++I) {
		switch (sqlite3_column_type(Stat->Handle, I)) {
		case SQLITE_NULL:
			ml_tuple_set(Tuple, I + 1, MLNil);
			break;
		case SQLITE_INTEGER:
			ml_tuple_set(Tuple, I + 1, ml_integer(sqlite3_column_int64(Stat->Handle, I)));
			break;
		case SQLITE_FLOAT:
			ml_tuple_set(Tuple, I + 1, ml_real(sqlite3_column_double(Stat->Handle, I)));
			break;
		case SQLITE_TEXT: {
			int Length = sqlite3_column_bytes(Stat->Handle, I);
			char *Value = snew(Length + 1);
			memcpy(Value, sqlite3_column_text(Stat->Handle, I), Length);
			Value[Length] = 0;
			ml_tuple_set(Tuple, I + 1, ml_string(Value, Length));
			break;
		}
		case SQLITE_BLOB: {
			int Length = sqlite3_column_bytes(Stat->Handle, I);
			char *Value = snew(Length + 1);
			memcpy(Value, sqlite3_column_blob(Stat->Handle, I), Length);
			Value[Length] = 0;
			ml_tuple_set(Tuple, I + 1, ml_string(Value, Length));
			break;
		}
		}
	}
	ML_RETURN(Tuple);
}

ML_METHOD("prepare", MLSqliteT, MLStringT) {
	ml_sqlite_t *Sqlite = (ml_sqlite_t *)Args[0];
	if (!Sqlite->Handle) return ml_error("SqliteError", "Connection already closed");
	ml_sqlite_stmt_t *Stat = new(ml_sqlite_stmt_t);
	Stat->Type = MLSqliteStmtT;
	int Status = sqlite3_prepare_v2(Sqlite->Handle, ml_string_value(Args[1]), ml_string_length(Args[1]), &Stat->Handle, NULL);
	if (Status != SQLITE_OK) {
		return ml_error("SqliteError", "Error opening database: %s", sqlite3_errmsg(Sqlite->Handle));
	}
	GC_register_finalizer(Stat, (void *)ml_sqlite_stat_finalize, NULL, NULL, NULL);
	return (ml_value_t *)Stat;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
#include "ml_sqlite_init.c"
	stringmap_insert(MLSqliteT->Exports, "open", MLSqliteOpenFlags);
	ml_module_export(Module, "sqlite", (ml_value_t *)MLSqliteT);
}
