#include "../ml_library.h"
#include "../ml_macros.h"
#include "../ml_object.h"
#include <string.h>
#include "zlog/src/zlog.h"
#include "zlog/src/category.h"

typedef struct {
	ml_type_t *Type;
	zlog_category_t *Category;
	int Level;
} ml_logger_t;

static void ml_logger_call(ml_state_t *Caller, ml_logger_t *Logger, int Count, ml_value_t **Args) {
	if (!zlog_category_needless_level(Logger->Category, Logger->Level)) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		for (int I = 0; I < Count; ++I) {
			ml_stringbuffer_append(Buffer, Args[I]);
		}
		ml_source_t Source = ml_debugger_source(Caller);
		zlog(Logger->Category, Source.Name, strlen(Source.Name), "", 0, Source.Line, Logger->Level, "%s", ml_stringbuffer_get_string(Buffer));
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLLoggerT, (), "logger",
	.call = (void *)ml_logger_call
);

static ml_logger_t *ml_logger(zlog_category_t *Category, int Level) {
	ml_logger_t *Logger = new(ml_logger_t);
	Logger->Type = MLLoggerT;
	Logger->Category = Category;
	Logger->Level = Level;
	return Logger;
}

typedef struct {
	ml_type_t *Type;
	zlog_category_t *Handle;
	stringmap_t Loggers[1];
} ml_category_t;

extern ml_type_t MLCategoryT[];

ML_FUNCTIONX(MLCategory) {
	const char *Name;
	if (Count > 0) {
		ML_CHECKX_ARG_TYPE(0, MLStringT);
		Name = ml_string_value(Args[0]);
	} else {
		Name = ml_debugger_source(Caller).Name;
	}
	ml_category_t *Category = new(ml_category_t);
	Category->Type = MLCategoryT;
	Category->Handle = zlog_get_category(Name);
	stringmap_insert(Category->Loggers, "debug", ml_logger(Category->Handle, ZLOG_LEVEL_DEBUG));
	stringmap_insert(Category->Loggers, "info", ml_logger(Category->Handle, ZLOG_LEVEL_INFO));
	stringmap_insert(Category->Loggers, "notice", ml_logger(Category->Handle, ZLOG_LEVEL_NOTICE));
	stringmap_insert(Category->Loggers, "warn", ml_logger(Category->Handle, ZLOG_LEVEL_WARN));
	stringmap_insert(Category->Loggers, "error", ml_logger(Category->Handle, ZLOG_LEVEL_ERROR));
	stringmap_insert(Category->Loggers, "fatal", ml_logger(Category->Handle, ZLOG_LEVEL_FATAL));
	ML_RETURN(Category);
}

ML_TYPE(MLCategoryT, (), "category",
	.Constructor = (ml_value_t *)MLCategory
);

ML_METHOD("::", MLCategoryT, MLStringT) {
	ml_category_t *Category = (ml_category_t *)Args[0];
	ml_logger_t *Logger = (ml_logger_t *)stringmap_search(Category->Loggers, ml_string_value(Args[1]));
	if (!Logger) return ml_error("NameError", "Unknown logging level");
	return (ml_value_t *)Logger;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
#include "ml_zlog_init.c"
	if (zlog_init("zlog.conf")) {
		printf("Failed to load zlog config\n");
	}
	ml_module_export(Module, "logger", (ml_value_t *)MLCategoryT);
}
