#include "ml_sqlite.h"
#include "ml_macros.h"
#include "ml_object.h"
#include <sqlite3.h>

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



void ml_sqlite_init(stringmap_t *Globals) {
#include "ml_sqlite_init.c"
	stringmap_insert(MLSqliteT->Exports, "open", MLSqliteOpenFlags);
	if (Globals) {
		stringmap_insert(Globals, "sqlite", MLSqliteT);
	}
}
