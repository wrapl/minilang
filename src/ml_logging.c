#include "ml_logging.h"
#include "ml_object.h"
#include "ml_macros.h"
#include "ml_compiler2.h"
#include <sys/time.h>
#include <math.h>

#undef ML_CATEGORY
#define ML_CATEGORY "logging"

ml_log_level_t MLLogLevel = ML_LOG_LEVEL_INFO;

static const char *MLLogLevelNames[] = {
	[ML_LOG_LEVEL_NONE] = "NONE",
	[ML_LOG_LEVEL_ERROR] = "\e[31mERROR\e[0m",
	[ML_LOG_LEVEL_WARN] = "\e[35mWARN\e[0m",
	[ML_LOG_LEVEL_INFO] = "\e[32mINFO\e[0m",
	[ML_LOG_LEVEL_DEBUG] = "\e[34mDEBUG\e[0m"
};

static void ml_log_default(ml_logger_t *Logger, ml_log_level_t Level, const char *Source, int Line, const char *Format, ...) {
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
}

ml_logger_fn ml_log = ml_log_default;
static ml_value_t *MLLog = NULL;

typedef struct {
	ml_state_t Base;
	ml_logger_t *Logger;
	const char *Source;
	ml_stringbuffer_t Buffer[1];
	ml_log_level_t Level;
	int Line, Index;
	ml_value_t *Args[];
} ml_log_state_t;

static int ml_log_buffer_fn(ml_log_state_t *State, const char *Chars, size_t Length) {
	ml_log(State->Logger, State->Level, State->Source, State->Line, "%.*s", (int)Length, Chars);
	return 0;
}

extern ml_value_t *AppendMethod;

static void ml_log_state_run(ml_log_state_t *State, ml_value_t *Value) {
	// TODO: Check for error
	ml_value_t *Arg = State->Args[State->Index];
	if (Arg) {
		ml_stringbuffer_put(State->Buffer, ' ');
		++State->Index;
		State->Args[1] = Arg;
		return ml_call(State, AppendMethod, 2, State->Args);
	}
	ml_stringbuffer_drain(State->Buffer, State, (void *)ml_log_buffer_fn);
	ML_CONTINUE(State->Base.Caller, MLNil);
}

static void ml_log_fn(ml_state_t *Caller, ml_logger_t *Logger, int Count, ml_value_t **Args) {
	ml_log_level_t Level = ml_integer_value(Args[0]);
	if (Level <= ML_LOG_LEVEL_NONE || Level > MLLogLevel || Logger->Ignored[Level]) ML_RETURN(MLNil);
	if (MLLog) {
		// TODO: Call MLLog
		ML_RETURN(MLNil);
	} else {
		ml_log_state_t *State = xnew(ml_log_state_t, Count + 1, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (ml_state_fn)ml_log_state_run;
		State->Logger = Logger;
		State->Buffer[0] = ML_STRINGBUFFER_INIT;
		ml_source_t Source = ml_debugger_source(Caller);
		State->Source = Source.Name;
		State->Line = Source.Line;
		State->Level = Level;
		for (int I = 1; I < Count; ++I) State->Args[I] = Args[I];
		if (Count == 0) return ml_log_state_run(State, MLNil);
		State->Index = 2;
		State->Args[0] = (ml_value_t *)State->Buffer;
		return ml_call(State, AppendMethod, 2, State->Args);
	}
}

#define ML_CONFIG_LOG_LEVEL(NAME, LEVEL) \
\
static int ml_config_log_ ## NAME(ml_context_t *Context) { \
	return MLLogLevel >= ML_LOG_LEVEL_ ## LEVEL; \
}

ML_CONFIG_LOG_LEVEL(error, ERROR);
ML_CONFIG_LOG_LEVEL(warn, WARN);
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

static ml_value_t *ml_log_macro_special(void *LogFn) {
	return (ml_value_t *)LogFn;
}

static ml_value_t *ml_log_macro(ml_logger_t *Logger, ml_value_t *LogFn, const char *Input) {
	ml_log_macro_t *Macro = new(ml_log_macro_t);
	Macro->Type = MLLogMacroT;
	ml_parser_t *Parser = ml_parser(NULL, NULL);
	ml_parser_input(Parser, Input);
	ml_parser_special(Parser, ml_log_macro_special, LogFn);
	Macro->Expr = ml_parse_expr(Parser);
	if (!Macro->Expr) return ml_parser_value(Parser);
	return ml_macro((ml_value_t *)Macro);
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
	Logger->Loggers[ML_LOG_LEVEL_ERROR] = ml_log_macro(Logger, LogFn, "ifConfig \"LOG>=ERROR\" \uFFFC(1, :$Args)");
	Logger->Loggers[ML_LOG_LEVEL_WARN] = ml_log_macro(Logger, LogFn, "ifConfig \"LOG>=WARN\" \uFFFC(2, :$Args)");
	Logger->Loggers[ML_LOG_LEVEL_INFO] = ml_log_macro(Logger, LogFn, "ifConfig \"LOG>=INFO\" \uFFFC(3, :$Args)");
	Logger->Loggers[ML_LOG_LEVEL_DEBUG] = ml_log_macro(Logger, LogFn, "ifConfig \"LOG>=DEBUG\" \uFFFC(4, :$Args)");
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
	if (!strcasecmp(Level, "error")) return Logger->Loggers[ML_LOG_LEVEL_ERROR];
	if (!strcasecmp(Level, "warn")) return Logger->Loggers[ML_LOG_LEVEL_WARN];
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
		if (!strcasecmp(Level, "info")) MLLogLevel = ML_LOG_LEVEL_INFO;
		if (!strcasecmp(Level, "debug")) MLLogLevel = ML_LOG_LEVEL_DEBUG;
		if (!strcasecmp(Level, "all")) MLLogLevel = ML_LOG_LEVEL_ALL;
	}
	switch (MLLogLevel) {
	case ML_LOG_LEVEL_ERROR: return ml_cstring("Error");
	case ML_LOG_LEVEL_WARN: return ml_cstring("Warn");
	case ML_LOG_LEVEL_INFO: return ml_cstring("Info");
	case ML_LOG_LEVEL_DEBUG: return ml_cstring("Debug");
	case ML_LOG_LEVEL_ALL: return ml_cstring("All");
	default: return MLNil;
	}
}

void ml_logging_init(stringmap_t *Globals) {
#include "ml_logging_init.c"
	ml_config_register("LOG>=ERROR", ml_config_log_error);
	ml_config_register("LOG>=WARN", ml_config_log_warn);
	ml_config_register("LOG>=INFO", ml_config_log_info);
	ml_config_register("LOG>=DEBUG", ml_config_log_debug);
	stringmap_insert(MLLoggerT->Exports, "level", MLLoggerLevel);
	if (Globals) {
		stringmap_insert(Globals, "logger", MLLoggerT);
	}
}
