#ifndef ML_LOGGER_H
#define ML_LOGGER_H

/// \defgroup logging
/// @{
///

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ML_LOG_LEVEL_NONE = 0,
	ML_LOG_LEVEL_FATAL = 1,
	ML_LOG_LEVEL_ERROR = 2,
	ML_LOG_LEVEL_WARN = 3,
	ML_LOG_LEVEL_INFO = 4,
	ML_LOG_LEVEL_DEBUG = 5,
	ML_LOG_LEVEL_ALL = 6
} ml_log_level_t;

typedef struct ml_logger_t ml_logger_t;

struct ml_logger_t {
	ml_type_t *Type;
	const char *Name;
	const char *AnsiName;
	ml_value_t *Loggers[ML_LOG_LEVEL_ALL];
	int Ignored[ML_LOG_LEVEL_ALL];
};

typedef void (*ml_logger_fn)(ml_logger_t *Logger, ml_log_level_t Level, ml_value_t *Error, const char *Source, int Line, const char *Format, ...) __attribute__((format(printf, 6, 7)));

extern ml_log_level_t MLLogLevel;
extern ml_logger_fn ml_log;
extern ml_logger_t MLLoggerDefault[];

#ifndef ML_LOGGER
#define ML_LOGGER MLLoggerDefault
#endif

#define ML_LOG_FATAL(ERROR, FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_FATAL) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_FATAL, ERROR, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__); \
	}
#define ML_LOG_ERROR(ERROR, FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_ERROR) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_ERROR, ERROR, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__); \
	}
#define ML_LOG_WARN(ERROR, FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_WARN) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_WARN, ERROR, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__); \
	}
#define ML_LOG_INFO(ERROR, FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_INFO) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_INFO, ERROR, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__); \
	}
#define ML_LOG_DEBUG(ERROR, FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_DEBUG) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_DEBUG, ERROR, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__); \
	}

ml_logger_t *ml_logger(const char *Name);
void ml_logger_init(ml_logger_t *Logger, const char *Name);

void ml_logging_init(stringmap_t *Globals);

typedef void (*ml_log_level_fn)(ml_log_level_t Level, void *Data);

void ml_log_level_watch(ml_log_level_fn Fn, void *Data);
void ml_log_level_set(ml_log_level_t Level);

#ifdef __cplusplus
}
#endif

/// @}

#endif
