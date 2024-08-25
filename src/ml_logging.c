#include "ml_logging.h"
#include "ml_object.h"
#include "ml_macros.h"
#include "ml_compiler2.h"
#include <sys/time.h>
#include <stdatomic.h>
#include <math.h>

#undef ML_CATEGORY
#define ML_CATEGORY "logging"

ml_log_level_t MLLogLevel = ML_LOG_LEVEL_INFO;

static const char *MLLogLevelNames[] = {
	[ML_LOG_LEVEL_NONE] = "NONE",
	[ML_LOG_LEVEL_FATAL] = "\e[31mFATAL\e[0m",
	[ML_LOG_LEVEL_ERROR] = "\e[31mERROR\e[0m",
	[ML_LOG_LEVEL_WARN] = "\e[35mWARN\e[0m",
	[ML_LOG_LEVEL_MESSAGE] = "\e[32mMESSAGE\e[0m",
	[ML_LOG_LEVEL_INFO] = "\e[32mINFO\e[0m",
	[ML_LOG_LEVEL_DEBUG] = "\e[34mDEBUG\e[0m"
};

static void ml_log_default(ml_logger_t *Logger, ml_log_level_t Level, ml_value_t *Error, const char *Source, int Line, const char *Format, ...) {
	struct timespec Time;
	clock_gettime(CLOCK_REALTIME, &Time);
	struct tm BrokenTime;
	gmtime_r(&Time.tv_sec, &BrokenTime);
	char TimeString[20];
	strftime(TimeString, 20, "%F %T", &BrokenTime);
	fprintf(stderr, "[%s] %s %s %s:%d ", MLLogLevelNames[Level], TimeString, Logger->AnsiName, Source, Line);
	va_list Args;
	va_start(Args, Format);
	vfprintf(stderr, Format, Args);
	va_end(Args);
	fprintf(stderr, "\n");
	if (Error) {
		fprintf(stderr, "\t%s: %s\n", ml_error_type(Error), ml_error_message(Error));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Error, Level++, &Source)) {
			fprintf(stderr, "\t\t%s:%d\n", Source.Name, Source.Line);
		}
	}
}

ml_logger_fn ml_log = ml_log_default;

typedef struct ml_log_state_t  ml_log_state_t;

struct ml_log_state_t {
	ml_state_t Base;
	union { ml_logger_t *Logger; ml_log_state_t *Next; };
	const char *Source;
	ml_stringbuffer_t Buffer[1];
	ml_log_level_t Level;
	int Line, Index;
	ml_value_t *Error;
	ml_value_t *Args[];
};

extern ml_value_t *AppendMethod;

#ifdef ML_THREADSAFE
static ml_log_state_t * _Atomic LogStateCache = NULL;
#else
static ml_log_state_t *LogStateCache = NULL;
#endif

#define MAX_LOG_ARG_COUNT 8

static void ml_log_state_run(ml_log_state_t *State, ml_value_t *Value) {
	ml_value_t *Arg = State->Args[State->Index];
	if (Arg) {
		if (ml_typeof(Arg) == MLErrorValueT) {
			State->Error = ml_error_value_error(Arg);
			++State->Index;
			return ml_log_state_run(State, MLNil);
		}
		ml_stringbuffer_put(State->Buffer, ' ');
		++State->Index;
		State->Args[0] = (ml_value_t *)State->Buffer;
		State->Args[1] = Arg;
		return ml_call(State, AppendMethod, 2, State->Args);
	}
	int Length = ml_stringbuffer_length(State->Buffer);
	if (!Length) {
		ml_log(State->Logger, State->Level, State->Error, State->Source, State->Line, "");
	} else if (Length < ML_STRINGBUFFER_NODE_SIZE - 1) {
		char *Message = State->Buffer->Head->Chars;
		Message[Length] = 0;
		ml_log(State->Logger, State->Level, State->Error, State->Source, State->Line, "%.*s", Length, Message);
		ml_stringbuffer_clear(State->Buffer);
	} else {
		const char *Message = ml_stringbuffer_get_string(State->Buffer);
		ml_log(State->Logger, State->Level, State->Error, State->Source, State->Line, "%.*s", Length, Message);
	}
	if (State->Index <= MAX_LOG_ARG_COUNT) {
		for (int I = 0; I < State->Index; ++I) State->Args[I] = NULL;
		State->Error = NULL;
#ifdef ML_THREADSAFE
		ml_log_state_t *CacheNext = LogStateCache;
		do {
			State->Next = CacheNext;
		} while (!atomic_compare_exchange_weak(&LogStateCache, &CacheNext, State));
#else
		State->Next = LogStateCache;
		LogStateCache = State;
#endif
	}
	ML_CONTINUE(State->Base.Caller, MLNil);
}

static ml_log_state_t *ml_log_state(int Count) {
	if (Count > MAX_LOG_ARG_COUNT) {
		ml_log_state_t *State = xnew(ml_log_state_t, Count + 1, ml_value_t *);
		State->Base.run = (ml_state_fn)ml_log_state_run;
		return State;
	}
#ifdef ML_THREADSAFE
	ml_log_state_t *Next = LogStateCache, *CacheNext;
	do {
		if (!Next) {
			Next = xnew(ml_log_state_t, MAX_LOG_ARG_COUNT + 1, ml_value_t *);
			break;
		}
		CacheNext = Next->Next;
	} while (!atomic_compare_exchange_weak(&LogStateCache, &Next, CacheNext));
#else
	ml_log_state_t *Next = LogStateCache;
	if (Next) {
		LogStateCache = Next->Next;
	} else {
		Next = xnew(ml_log_state_t, MAX_LOG_ARG_COUNT + 1, ml_value_t *);
		Next->Base.run = (ml_state_fn)ml_log_state_run;
	}
#endif
	return Next;
}

static void ml_log_fn(ml_state_t *Caller, ml_logger_t *Logger, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ml_log_level_t Level = ml_integer_value(Args[0]);
	if (Level <= ML_LOG_LEVEL_NONE || Level > MLLogLevel || Logger->Ignored[Level]) ML_RETURN(MLNil);
	ml_source_t Source = ml_debugger_source(Caller);
	ml_log_state_t *State = ml_log_state(Count);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_log_state_run;
	State->Logger = Logger;
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Source = Source.Name;
	State->Line = Source.Line;
	State->Level = Level;
	for (int I = 1; I < Count; ++I) State->Args[I] = ml_deref(Args[I]);
	State->Index = 2;
	State->Args[0] = (ml_value_t *)State->Buffer;
	if (ml_typeof(State->Args[1]) == MLErrorValueT) {
		State->Error = ml_error_value_error(State->Args[1]);
		return ml_log_state_run(State, MLNil);
	}
	return ml_call(State, AppendMethod, 2, State->Args);
}

#define ML_CONFIG_LOG_LEVEL(NAME, LEVEL) \
\
static int ml_config_log_ ## NAME(ml_context_t *Context) { \
	return MLLogLevel >= ML_LOG_LEVEL_ ## LEVEL; \
}

ML_CONFIG_LOG_LEVEL(fatal, FATAL);
ML_CONFIG_LOG_LEVEL(error, ERROR);
ML_CONFIG_LOG_LEVEL(warn, WARN);
ML_CONFIG_LOG_LEVEL(message, MESSAGE);
ML_CONFIG_LOG_LEVEL(info, INFO);
ML_CONFIG_LOG_LEVEL(debug, DEBUG);

extern ml_type_t MLLoggerT[];

typedef struct {
	ml_type_t *Type;
	mlc_expr_t *Expr;
} ml_log_macro_t;

static void ml_log_macro_call(ml_state_t *Caller, ml_log_macro_t *Macro, int Count, ml_value_t **Args) {
	ml_value_t *List = ml_list();
	for (int I = 0; I < Count; ++I) ml_list_put(List, Args[I]);
	const char *Names[1] = {"Args"};
	ml_value_t *Exprs[1] = {List};
	ML_RETURN(ml_macro_subst(Macro->Expr, 1, Names, Exprs));
}

ML_TYPE(MLLogMacroT, (MLFunctionT), "log::macro",
	.call = (void *)ml_log_macro_call
);

typedef struct {
	ml_value_t *LogFn;
	int Level;
} ml_log_info_t;

static mlc_expr_t *ml_log_macro_fn(mlc_expr_t *Expr, mlc_expr_t *Child, ml_log_info_t *Info) {
	static const char *Conditions[] = {
		[ML_LOG_LEVEL_NONE] = "",
		[ML_LOG_LEVEL_FATAL] = "LOG>=FATAL",
		[ML_LOG_LEVEL_ERROR] = "LOG>=ERROR",
		[ML_LOG_LEVEL_WARN] = "LOG>=WARN",
		[ML_LOG_LEVEL_MESSAGE] = "LOG>=MESSAGE",
		[ML_LOG_LEVEL_INFO] = "LOG>=INFO",
		[ML_LOG_LEVEL_DEBUG] = "LOG>=DEBUG"
	};

	mlc_value_expr_t *LevelExpr = new(mlc_value_expr_t);
	LevelExpr->compile = ml_value_expr_compile;
	LevelExpr->Source = Expr->Source;
	LevelExpr->StartLine = Expr->StartLine;
	LevelExpr->EndLine = Expr->EndLine;
	LevelExpr->Value = ml_integer(Info->Level);
	LevelExpr->Next = Child;

	mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
	CallExpr->compile = ml_const_call_expr_compile;
	CallExpr->Source = Expr->Source;
	CallExpr->StartLine = Expr->StartLine;
	CallExpr->EndLine = Expr->EndLine;
	CallExpr->Value = Info->LogFn;
	CallExpr->Child = (mlc_expr_t *)LevelExpr;

	mlc_if_config_expr_t *IfConfigExpr = new(mlc_if_config_expr_t);
	IfConfigExpr->compile = ml_if_config_expr_compile;
	IfConfigExpr->Source = Expr->Source;
	IfConfigExpr->StartLine = Expr->StartLine;
	IfConfigExpr->EndLine = Expr->EndLine;
	IfConfigExpr->Config = Conditions[Info->Level];
	IfConfigExpr->Child = (mlc_expr_t *)CallExpr;

	return (mlc_expr_t *)IfConfigExpr;
}

static ml_value_t *ml_log_macro(ml_value_t *LogFn, int Level) {
	ml_log_info_t *Info = new(ml_log_info_t);
	Info->LogFn = LogFn;
	Info->Level = Level;
	return ml_macrox((void *)ml_log_macro_fn, Info);
}

void ml_logger_init(ml_logger_t *Logger, const char *Name) {
	Logger->Type = MLLoggerT;
	Logger->Name = Name;
	int Hue = 0;
	for (const unsigned char *P = (const unsigned char *)Name; *P; ++P) Hue = (7 * Hue + 13 * P[0]) % 360;
	double H = fmod(Hue / 60.0, 2.0);
	int X = round(255 * (1 - fabs(H - 1)));
	int R, G, B;
	switch ((int)H) {
	case 0: R = 255; G = X; B = 0; break;
	case 1: R = X; G = 255; B = 0; break;
	case 2: R = 0; G = 255; B = X; break;
	case 3: R = 0; G = X; B = 255; break;
	case 4: R = X; G = 0; B = 255; break;
	default: R = 255; G = 0; B = X; break;
	}
	GC_asprintf((char **)&Logger->AnsiName, "\e[38;2;%d;%d;%dm%s\e[0m", R, G, B, Name);
	ml_value_t *LogFn = ml_cfunctionx(Logger, (ml_callbackx_t)ml_log_fn);
	Logger->Loggers[ML_LOG_LEVEL_FATAL] = ml_log_macro(LogFn, ML_LOG_LEVEL_FATAL);
	Logger->Loggers[ML_LOG_LEVEL_ERROR] = ml_log_macro(LogFn, ML_LOG_LEVEL_ERROR);
	Logger->Loggers[ML_LOG_LEVEL_WARN] = ml_log_macro(LogFn, ML_LOG_LEVEL_WARN);
	Logger->Loggers[ML_LOG_LEVEL_MESSAGE] = ml_log_macro(LogFn, ML_LOG_LEVEL_MESSAGE);
	Logger->Loggers[ML_LOG_LEVEL_INFO] = ml_log_macro(LogFn, ML_LOG_LEVEL_INFO);
	Logger->Loggers[ML_LOG_LEVEL_DEBUG] = ml_log_macro(LogFn, ML_LOG_LEVEL_DEBUG);
}

ml_logger_t *ml_logger(const char *Name) {
	static stringmap_t Loggers[1] = {STRINGMAP_INIT};
	ml_logger_t **Slot = (ml_logger_t **)stringmap_slot(Loggers, Name);
	if (!Slot[0]) {
		ml_logger_t *Logger = new(ml_logger_t);
		ml_logger_init(Logger, Name);
		Slot[0] = Logger;
	}
	return Slot[0];
}

ML_FUNCTION(MLLogger) {
//@logger
//<Category
//>logger
// Returns a new logger with levels :mini:`::error`, :mini:`::warn`, :mini:`::info` and :mini:`::debug`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return (ml_value_t *)ml_logger(ml_string_value(Args[0]));
}

ML_TYPE(MLLoggerT, (), "logger",
// A logger.
	.Constructor = (ml_value_t *)MLLogger
);

ML_METHOD("::", MLLoggerT, MLStringT) {
//<Logger
//<Level
//>logger::fn
	ml_logger_t *Logger = (ml_logger_t *)Args[0];
	const char *Level = ml_string_value(Args[1]);
	if (!strcasecmp(Level, "fatal")) return Logger->Loggers[ML_LOG_LEVEL_FATAL];
	if (!strcasecmp(Level, "error")) return Logger->Loggers[ML_LOG_LEVEL_ERROR];
	if (!strcasecmp(Level, "warn")) return Logger->Loggers[ML_LOG_LEVEL_WARN];
	if (!strcasecmp(Level, "message")) return Logger->Loggers[ML_LOG_LEVEL_MESSAGE];
	if (!strcasecmp(Level, "info")) return Logger->Loggers[ML_LOG_LEVEL_INFO];
	if (!strcasecmp(Level, "debug")) return Logger->Loggers[ML_LOG_LEVEL_DEBUG];
	return ml_error("NameError", "Unknown log level %s", Level);
}

ML_FUNCTION(MLLoggerLevel) {
//@logger::level
//<Level?:string
//>string
// Gets or sets the logging level for default logging. Returns the log level.
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLStringT);
		const char *Level = ml_string_value(Args[0]);
		if (!strcasecmp(Level, "error")) MLLogLevel = ML_LOG_LEVEL_ERROR;
		if (!strcasecmp(Level, "warn")) MLLogLevel = ML_LOG_LEVEL_WARN;
		if (!strcasecmp(Level, "message")) MLLogLevel = ML_LOG_LEVEL_MESSAGE;
		if (!strcasecmp(Level, "info")) MLLogLevel = ML_LOG_LEVEL_INFO;
		if (!strcasecmp(Level, "debug")) MLLogLevel = ML_LOG_LEVEL_DEBUG;
		if (!strcasecmp(Level, "all")) MLLogLevel = ML_LOG_LEVEL_ALL;
	}
	switch (MLLogLevel) {
	case ML_LOG_LEVEL_FATAL: return ml_cstring("Fatal");
	case ML_LOG_LEVEL_ERROR: return ml_cstring("Error");
	case ML_LOG_LEVEL_WARN: return ml_cstring("Warn");
	case ML_LOG_LEVEL_MESSAGE: return ml_cstring("Message");
	case ML_LOG_LEVEL_INFO: return ml_cstring("Info");
	case ML_LOG_LEVEL_DEBUG: return ml_cstring("Debug");
	case ML_LOG_LEVEL_ALL: return ml_cstring("All");
	default: return MLNil;
	}
}

ml_logger_t MLLoggerDefault[1];

typedef struct ml_log_level_watch_t ml_log_level_watch_t;

struct ml_log_level_watch_t {
	ml_log_level_watch_t *Next;
	ml_log_level_fn Fn;
	void *Data;
};

static ml_log_level_watch_t *MLLogLevelWatches = NULL;

void ml_log_level_watch(ml_log_level_fn Fn, void *Data) {
	ml_log_level_watch_t *Watch = new(ml_log_level_watch_t);
	Watch->Fn = Fn;
	Watch->Data = Data;
	Watch->Next = MLLogLevelWatches;
	MLLogLevelWatches = Watch;
}

void ml_log_level_set(ml_log_level_t Level) {
	MLLogLevel = Level;
	for (ml_log_level_watch_t *Watch = MLLogLevelWatches; Watch; Watch = Watch->Next) {
		Watch->Fn(Level, Watch->Data);
	}
}

void ml_logging_init(stringmap_t *Globals) {
#include "ml_logging_init.c"
	ml_logger_init(MLLoggerDefault, "main");
	ml_config_register("LOG>=FATAL", ml_config_log_fatal);
	ml_config_register("LOG>=ERROR", ml_config_log_error);
	ml_config_register("LOG>=WARN", ml_config_log_warn);
	ml_config_register("LOG>=MESSAGE", ml_config_log_message);
	ml_config_register("LOG>=INFO", ml_config_log_info);
	ml_config_register("LOG>=DEBUG", ml_config_log_debug);
	stringmap_insert(MLLoggerT->Exports, "level", MLLoggerLevel);
	if (Globals) {
		stringmap_insert(Globals, "logger", MLLoggerT);
	}
}
