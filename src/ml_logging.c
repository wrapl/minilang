#include "ml_logging.h"
#include "ml_object.h"
#include "ml_macros.h"
#include "ml_compiler2.h"
#include <sys/time.h>

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
	fprintf(stderr, "[%s] %s %s %s:%d ", MLLogLevelNames[Level], TimeString, Logger->Name, Source, Line);
	va_list Args;
	va_start(Args, Format);
	vfprintf(stderr, Format, Args);
	va_end(Args);
}

ml_logger_fn ml_log = ml_log_default;
static ml_value_t *MLLog = NULL;

typedef struct {
	ml_logger_t *Logger;
	ml_log_level_t Level;
	const char *Source;
	int Line;
} ml_log_info_t;

static int ml_log_buffer_fn(ml_log_info_t *Info, const char *Chars, size_t Length) {
	ml_log(Info->Logger, Info->Level, Info->Source, Info->Line, "%.*s", (int)Length, Chars);
	return 0;
}

static void ml_log_fn(ml_state_t *Caller, ml_logger_t *Logger, int Count, ml_value_t **Args) {
	ml_log_level_t Level = ml_integer_value(Args[0]);
	if (Level < MLLogLevel || Logger->Ignored[Level]) ML_RETURN(MLNil);
	if (MLLog) {
		// TODO: Call MLLog
		ML_RETURN(MLNil);
	} else {
		ml_source_t Source = ml_debugger_source(Caller);
		ml_log_info_t Info = {Logger, Level, Source.Name, Source.Line};
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_drain(Buffer, &Info, (void *)ml_log_buffer_fn);
		ML_RETURN(MLNil);
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

} ml_log_macro_t;

static void ml_log_macro_call(ml_state_t *Caller, ml_log_macro_t *Macro, int Count, ml_value_t **Args) {

}

ML_TYPE(MLLogMacroT, (MLFunctionT), "log::macro",
	.call = (void *)ml_log_macro_call
);

static ml_value_t *ml_log_macro(ml_logger_t *Logger, ml_log_level_t Level) {
	ml_log_macro_t *Macro = new(ml_log_macro_t);
	Macro->Type = MLLogMacroT;
	return ml_macro((ml_value_t *)Macro);
}

ML_FUNCTION(MLLogger) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_logger_t *Logger = new(ml_logger_t);
	Logger->Type = MLLoggerT;
	Logger->Name = ml_string_value(Args[0]);
	ml_value_t *LogFn = ml_cfunctionx(Logger, (ml_callbackx_t)ml_log_fn);


	return (ml_value_t *)Logger;
}

ML_TYPE(MLLoggerT, (), "logger",
	.Constructor = (ml_value_t *)MLLogger
);

void ml_logging_init(stringmap_t *Globals) {
#include "ml_logging_init.c"
	ml_config_register("LOG>=ERROR", ml_config_log_error);
	ml_config_register("LOG>=WARN", ml_config_log_warn);
	ml_config_register("LOG>=INFO", ml_config_log_info);
	ml_config_register("LOG>=DEBUG", ml_config_log_debug);
	ml_parser_t *Parser = ml_parser(NULL, NULL);
	ml_parser_input(Parser, "__if_config__ \"LOG>=ERROR\" :$Logger(:$$Args)");
}
