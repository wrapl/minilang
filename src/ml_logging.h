#ifndef ML_LOGGER_H
#define ML_LOGGER_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ML_LOG_LEVEL_NONE = 0,
	ML_LOG_LEVEL_ERROR = 1,
	ML_LOG_LEVEL_WARN = 2,
	ML_LOG_LEVEL_INFO = 3,
	ML_LOG_LEVEL_DEBUG = 4,
	ML_LOG_LEVEL_ALL = 5
} ml_log_level_t;

typedef struct ml_logger_t ml_logger_t;

struct ml_logger_t {
	ml_type_t *Type;
	const char *Name;
	const char *AnsiName;
	ml_value_t *Loggers[ML_LOG_LEVEL_ALL];
	int Ignored[ML_LOG_LEVEL_ALL];
};

typedef void (*ml_logger_fn)(ml_logger_t *Logger, ml_log_level_t Level, const char *Source, int Line, const char *Format, ...) __attribute__((format(printf, 5, 6)));

extern ml_log_level_t MLLogLevel;
extern ml_logger_fn ml_log;
extern ml_logger_t MLLoggerDefault[];

#define ML_LOGGER MLLoggerDefault

#define ML_LOG_ERROR(FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_ERROR) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_ERROR, __FILE__, __LINE__, FORMAT, __VA_ARGS__); \
	}
#define ML_LOG_WARN(FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_WARN) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_WARN, __FILE__, __LINE__, FORMAT, __VA_ARGS__); \
	}
#define ML_LOG_INFO(FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_INFO) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_INFO, __FILE__, __LINE__, FORMAT, __VA_ARGS__); \
	}
#define ML_LOG_DEBUG(FORMAT, ...) \
	if (MLLogLevel >= ML_LOG_LEVEL_DEBUG) { \
		ml_log(ML_LOGGER, ML_LOG_LEVEL_DEBUG, __FILE__, __LINE__, FORMAT, __VA_ARGS__); \
	}

ml_logger_t *ml_logger(const char *Name);

void ml_logging_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif
