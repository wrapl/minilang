#include "ml_time.h"
#include "ml_macros.h"
#include "ml_object.h"
#include "ml_compiler2.h"
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifdef ML_TIMEZONES
#include "timelib/timelib.h"
#endif

#ifdef ML_CBOR
#include "ml_cbor.h"
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "time"

// Overview
// Provides time and date operations.

typedef struct {
	const ml_type_t *Type;
	struct timespec Value[1];
} ml_time_t;

static long ml_time_hash(ml_time_t *Time, ml_hash_chain_t *Chain) {
	return (long)Time->Value->tv_sec;
}

ML_TYPE(MLTimeT, (), "time",
// An instant in time with nanosecond resolution.
	.hash = (void *)ml_time_hash
);

void ml_time_value(ml_value_t *Value, struct timespec *Time) {
	Time[0] = ((ml_time_t *)Value)->Value[0];
}

static int ML_TYPED_FN(ml_value_is_constant, MLTimeT, ml_value_t *Value) {
	return 1;
}

ML_METHOD(MLTimeT) {
//>time
// Returns the current time.
//$= time()
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	clock_gettime(CLOCK_REALTIME, Time->Value);
	return (ml_value_t *)Time;
}

ml_value_t *ml_time(time_t Sec, unsigned long NSec) {
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = Sec;
	Time->Value->tv_nsec = NSec;
	return (ml_value_t *)Time;
}

ml_value_t *ml_time_parse(const char *Value, int Length) {
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	struct tm TM = {0,};
	int Local = 1;
	int Offset = 0;
	if (Length > 10) {
		const char *Rest = strptime(Value, "%FT%T", &TM);
		if (!Rest) Rest = strptime(Value, "%F %T", &TM);
		if (!Rest) return ml_error("TimeError", "Error parsing time");
		if (Rest[0] == '.') {
			++Rest;
			char *End;
			unsigned long NSec = strtoul(Rest, &End, 10);
			for (int I = 9 - (End - Rest); --I >= 0;) NSec *= 10;
			Time->Value->tv_nsec = NSec;
			Rest = End;
		}
		if (Rest[0] == 'Z') {
			Local = 0;
		} else if (Rest[0] == '+' || Rest[0] == '-') {
			Local = 0;
			const char *Next = Rest + 1;
			if (!isdigit(Next[0]) || !isdigit(Next[1])) return ml_error("TimeError", "Error parsing time");
			Offset = 3600 * (10 * (Next[0] - '0') + (Next[1] - '0'));
			Next += 2;
			if (Next[0] == ':') ++Next;
			if (isdigit(Next[0]) && isdigit(Next[1])) {
				Offset += 60 * (10 * (Next[0] - '0') + (Next[1] - '0'));
			}
			if (Rest[0] == '-') Offset = -Offset;
		}
	} else {
		if (!strptime(Value, "%F", &TM)) return ml_error("TimeError", "Error parsing time");
	}
	if (Local) {
		Time->Value->tv_sec = mktime(&TM);
	} else {
		Time->Value->tv_sec = timegm(&TM) - Offset;
	}
	return (ml_value_t *)Time;
}

ml_value_t *ml_parser_escape_time(ml_parser_t *Parser) {
	const char *Next = ml_parser_clear(Parser);
	char Quote = *Next++;
	const char *End = Next;
	while (End[0] != Quote) {
		if (!End[0]) {
			ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
			break;
		}
		++End;
	}
	int Length = End - Next;
	ml_parser_input(Parser, End + 1, 0);
	ml_value_t *Value = ml_time_parse(Next, Length);
	if (ml_is_error(Value)) return Value;
	mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
	ValueExpr->compile = ml_value_expr_compile;
	ml_source_t Source = ml_parser_position(Parser);
	ValueExpr->Source = Source.Name;
	ValueExpr->StartLine = ValueExpr->EndLine = Source.Line;
	ValueExpr->Value = Value;
	return ml_expr_value((mlc_expr_t *)ValueExpr);
}

static void ML_TYPED_FN(ml_value_sha256, MLTimeT, ml_time_t *Value, ml_hash_chain_t *Chain, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	memset(Hash, 0, SHA256_BLOCK_SIZE);
	if (sizeof(struct timespec) <= SHA256_BLOCK_SIZE) {
		memcpy(Hash, Value->Value, sizeof(struct timespec));
	} else {
		memcpy(Hash, Value->Value, SHA256_BLOCK_SIZE);
	}
}

ML_METHOD(MLTimeT, MLStringT) {
//<String
//>time
// Parses the :mini:`String` as a time according to ISO 8601.
//$= time("2023-02-09T21:19:33.196413266")
	return ml_time_parse(ml_string_value(Args[0]), ml_string_length(Args[0]));
}

ML_METHOD(MLTimeT, MLStringT, MLStringT) {
//<String
//<Format
//>time
// Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = mktime(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLStringT, MLStringT, MLNilT) {
//<String
//<Format
//<TimeZone
//>time
// Parses the :mini:`String` as a time according to specified format. The time is assumed to be in UTC.
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT) {
//<Year
//<Month
//<Day
//<Hour
//<Minute
//<Second
//>time
// Returns the time specified by the provided components in the local time.
	struct tm TM = {0,};
	TM.tm_year = ml_integer_value(Args[0]) - 1900;
	TM.tm_mon = ml_integer_value(Args[1]) - 1;
	TM.tm_mday = ml_integer_value(Args[2]);
	TM.tm_hour = ml_integer_value(Args[3]);
	TM.tm_min = ml_integer_value(Args[4]);
	TM.tm_sec = ml_integer_value(Args[5]);
	TM.tm_isdst = -1;
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = mktime(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLNilT) {
//<Year
//<Month
//<Day
//<Hour
//<Minute
//<Second
//<TimeZone
//>time
// Returns the time specified by the provided components in UTC.
	struct tm TM = {0,};
	TM.tm_year = ml_integer_value(Args[0]) - 1900;
	TM.tm_mon = ml_integer_value(Args[1]) - 1;
	TM.tm_mday = ml_integer_value(Args[2]);
	TM.tm_hour = ml_integer_value(Args[3]);
	TM.tm_min = ml_integer_value(Args[4]);
	TM.tm_sec = ml_integer_value(Args[5]);
	TM.tm_isdst = -1;
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT) {
//<Year
//<Month
//<Day
//>time
// Returns the time specified by the provided components in the local time.
	struct tm TM = {0,};
	TM.tm_year = ml_integer_value(Args[0]) - 1900;
	TM.tm_mon = ml_integer_value(Args[1]) - 1;
	TM.tm_mday = ml_integer_value(Args[2]);
	TM.tm_isdst = -1;
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = mktime(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT, MLNilT) {
//<Year
//<Month
//<Day
//<TimeZone
//>time
// Returns the time specified by the provided components in UTC.
	struct tm TM = {0,};
	TM.tm_year = ml_integer_value(Args[0]) - 1900;
	TM.tm_mon = ml_integer_value(Args[1]) - 1;
	TM.tm_mday = ml_integer_value(Args[2]);
	TM.tm_isdst = -1;
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHODV("with", MLTimeT, MLNamesT) {
//<Time
//<Component,Value
//>time
// Returns :mini:`Time` with the the specified components updated.
	ML_NAMES_CHECK_ARG_COUNT(1);
	for (int I = 2; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLIntegerT);
	ml_time_t *Time = (ml_time_t *)Args[0];
	struct tm TM = {0,};
	localtime_r(&Time->Value->tv_sec, &TM);
	ml_value_t **Arg = Args + 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		const char *Part = ml_string_value(Iter->Value);
		if (!strcmp(Part, "year")) {
			TM.tm_year = ml_integer_value(*Arg++) - 1900;
		} else if (!strcmp(Part, "month")) {
			TM.tm_mon = ml_integer_value(*Arg++) - 1;
		} else if (!strcmp(Part, "day")) {
			TM.tm_mday = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "hour")) {
			TM.tm_hour = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "minute")) {
			TM.tm_min = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "second")) {
			TM.tm_sec = ml_integer_value(*Arg++);
		} else {
			return ml_error("ValueError", "Unknown time component %s", Part);
		}
	}
	Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = mktime(&TM);
	return (ml_value_t *)Time;
}

ML_METHODV("with", MLTimeT, MLNilT, MLNamesT) {
//<Time
//<TimeZone
//<Component,Value
//>time
// Returns :mini:`Time` with the the specified components updated in UTC.
	ML_NAMES_CHECK_ARG_COUNT(2);
	for (int I = 3; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLIntegerT);
	ml_time_t *Time = (ml_time_t *)Args[0];
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	ml_value_t **Arg = Args + 3;
	ML_NAMES_FOREACH(Args[2], Iter) {
		const char *Part = ml_string_value(Iter->Value);
		if (!strcmp(Part, "year")) {
			TM.tm_year = ml_integer_value(*Arg++) - 1900;
		} else if (!strcmp(Part, "month")) {
			TM.tm_mon = ml_integer_value(*Arg++) - 1;
		} else if (!strcmp(Part, "day")) {
			TM.tm_mday = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "hour")) {
			TM.tm_hour = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "minute")) {
			TM.tm_min = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "second")) {
			TM.tm_sec = ml_integer_value(*Arg++);
		} else {
			return ml_error("ValueError", "Unknown time component %s", Part);
		}
	}
	Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD("nsec", MLTimeT) {
//<Time
//>integer
// Returns the nanoseconds component of :mini:`Time`.
	ml_time_t *Time = (ml_time_t *)Args[0];
	return ml_integer(Time->Value->tv_nsec);
}

ML_METHOD("append", MLStringBufferT, MLTimeT) {
//<Buffer
//<Time
//>string
// Formats :mini:`Time` as a local time.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	struct tm TM = {0,};
	localtime_r(&Time->Value->tv_sec, &TM);
	char Temp[60];
	size_t Length;
	unsigned long NSec = Time->Value->tv_nsec / 1000;
	if (NSec) {
		int Width = 6;
		while (NSec % 10 == 0) {
			--Width;
			NSec /= 10;
		}
		Length = strftime(Temp, 40, "%FT%T", &TM);
		Length += sprintf(Temp + Length, ".%0*lu", Width, NSec);
	} else {
		Length = strftime(Temp, 60, "%FT%T", &TM);
	}
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLTimeT, MLNilT) {
//<Buffer
//<Time
//<TimeZone
//>string
// Formats :mini:`Time` as a UTC time according to ISO 8601.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	char Temp[60];
	size_t Length;
	unsigned long NSec = Time->Value->tv_nsec / 1000;
	if (NSec) {
		int Width = 6;
		while (NSec % 10 == 0) {
			--Width;
			NSec /= 10;
		}
		Length = strftime(Temp, 40, "%FT%T", &TM);
		Length += sprintf(Temp + Length, ".%0*luZ", Width, NSec);
	} else {
		Length = strftime(Temp, 60, "%FT%TZ", &TM);
	}
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLTimeT, MLStringT) {
//<Buffer
//<Time
//<Format
//>string
// Formats :mini:`Time` as a local time according to the specified format.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	const char *Format = ml_string_value(Args[2]);
	struct tm TM = {0,};
	localtime_r(&Time->Value->tv_sec, &TM);
	char Temp[120];
	size_t Length = strftime(Temp, 120, Format, &TM);
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLTimeT, MLStringT, MLNilT) {
//<Buffer
//<Time
//<Format
//<TimeZone
//>string
// Formats :mini:`Time` as a UTC time according to the specified format.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	const char *Format = ml_string_value(Args[2]);
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	char Temp[120];
	size_t Length = strftime(Temp, 120, Format, &TM);
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

static int ml_time_compare(ml_time_t *TimeA, ml_time_t *TimeB) {
	double Diff = difftime(TimeA->Value->tv_sec, TimeB->Value->tv_sec);
	if (Diff < 0) return -1;
	if (Diff > 0) return 1;
	if (TimeA->Value->tv_nsec < TimeB->Value->tv_nsec) return -1;
	if (TimeA->Value->tv_nsec > TimeB->Value->tv_nsec) return 1;
	return 0;
}

ML_METHOD("<>", MLTimeT, MLTimeT) {
//<A
//<B
//>integer
// Compares the times :mini:`A` and :mini:`B` and returns :mini:`-1`, :mini:`0` or :mini:`1` respectively.
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	ml_time_t *TimeB = (ml_time_t *)Args[1];
	return ml_integer(ml_time_compare(TimeA, TimeB));
}

#define ml_comp_method_time_time(NAME, SYMBOL) \
	ML_METHOD(NAME, MLTimeT, MLTimeT) { \
		ml_time_t *TimeA = (ml_time_t *)Args[0]; \
		ml_time_t *TimeB = (ml_time_t *)Args[1]; \
		return ml_time_compare(TimeA, TimeB) SYMBOL 0 ? Args[1] : MLNil; \
	}

ml_comp_method_time_time("=", ==);
ml_comp_method_time_time("!=", !=);
ml_comp_method_time_time("<", <);
ml_comp_method_time_time(">", >);
ml_comp_method_time_time("<=", <=);
ml_comp_method_time_time(">=", >=);

ML_METHOD("-", MLTimeT, MLTimeT) {
//<End
//<Start
//>real
// Returns the time elasped betwen :mini:`Start` and :mini:`End` in seconds.
//$= time("2022-04-01 12:00:00") - time("2022-04-01 11:00:00")
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	ml_time_t *TimeB = (ml_time_t *)Args[1];
	double Sec = difftime(TimeA->Value->tv_sec, TimeB->Value->tv_sec);
	double NSec = ((double)TimeA->Value->tv_nsec - (double)TimeB->Value->tv_nsec) / 1000000000.0;
	return ml_real(Sec + NSec);
}

ML_METHOD("+", MLTimeT, MLNumberT) {
//<Start
//<Duration
//>time
// Returns the time :mini:`Duration` seconds after :mini:`Start`.
//$= time("2022-04-01 12:00:00") + 3600
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	double Diff = ml_real_value(Args[1]);
	double DiffSec = floor(Diff);
	unsigned long DiffNSec = (Diff - DiffSec) * 1000000000.0;
	time_t Sec = TimeA->Value->tv_sec + DiffSec;
	unsigned long NSec = TimeA->Value->tv_nsec + DiffNSec;
	if (NSec >= 1000000000) {
		NSec -= 1000000000;
		++Sec;
	}
	return ml_time(Sec, NSec);
}

ML_METHOD("-", MLTimeT, MLNumberT) {
//<Start
//<Duration
//>time
// Returns the time :mini:`Duration` seconds before :mini:`Start`.
//$= time("2022-04-01 12:00:00") - 3600
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	double Diff = -ml_real_value(Args[1]);
	double DiffSec = floor(Diff);
	unsigned long DiffNSec = (Diff - DiffSec) * 1000000000.0;
	time_t Sec = TimeA->Value->tv_sec + DiffSec;
	unsigned long NSec = TimeA->Value->tv_nsec + DiffNSec;
	if (NSec >= 1000000000) {
		NSec -= 1000000000;
		++Sec;
	}
	return ml_time(Sec, NSec);
}

ML_METHOD("precision", MLTimeT, MLIntegerT) {
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	time_t Sec = TimeA->Value->tv_sec;
	unsigned long NSec = TimeA->Value->tv_nsec;
	switch (ml_integer_value(Args[1])) {
	case 0: NSec = 0; break;
	case 1: NSec = (NSec / 100000000) * 100000000; break;
	case 2: NSec = (NSec / 10000000) * 10000000; break;
	case 3: NSec = (NSec / 1000000) * 1000000; break;
	case 4: NSec = (NSec / 100000) * 100000; break;
	case 5: NSec = (NSec / 10000) * 10000; break;
	case 6: NSec = (NSec / 1000) * 1000; break;
	case 7: NSec = (NSec / 100) * 100; break;
	case 8: NSec = (NSec / 10) * 10; break;
	default: break;
	}
	return ml_time(Sec, NSec);
}

ML_ENUM_CYCLIC(MLTimeDayT, "time::day",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
);

ML_SUB_ENUM(MLTimeWeekdayT, "time::weekday", MLTimeDayT,
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday"
);

ML_SUB_ENUM(MLTimeWeekendT, "time::weekend", MLTimeDayT,
	"Saturday",
	"Sunday"
);

ML_ENUM_CYCLIC(MLTimeMonthT, "time::month",
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
);

#define ML_TIME_PART(NAME, DESC, EXPR) \
\
ML_METHOD(NAME, MLTimeT) { \
/*<Time
//>integer
// Returns the DESC from :mini:`Time` in local time.
*/ \
	ml_time_t *Time = (ml_time_t *)Args[0]; \
	struct tm TM; \
	localtime_r(&Time->Value->tv_sec, &TM); \
	return EXPR; \
} \
\
ML_METHOD(NAME, MLTimeT, MLNilT) { \
/*<Time
//<TimeZone
//>integer
// Returns the DESC from :mini:`Time` in UTC.
*/ \
	ml_time_t *Time = (ml_time_t *)Args[0]; \
	struct tm TM; \
	gmtime_r(&Time->Value->tv_sec, &TM); \
	return EXPR; \
}

ML_TIME_PART("year", year, ml_integer(TM.tm_year + 1900))
ML_TIME_PART("month", month, ml_enum_value(MLTimeMonthT, TM.tm_mon + 1))
ML_TIME_PART("day", date, ml_integer(TM.tm_mday))
ML_TIME_PART("yday", number of days from the start of the year, ml_integer(TM.tm_yday + 1))
ML_TIME_PART("wday", day of the week, ml_enum_value(MLTimeDayT, TM.tm_wday ?: 7))
ML_TIME_PART("hour", hour, ml_integer(TM.tm_hour))
ML_TIME_PART("minute", minute, ml_integer(TM.tm_min))
ML_TIME_PART("second", second, ml_integer(TM.tm_sec))

ML_FUNCTION(MLTimeMdays) {
//@time::mdays
//<Year
//<Month
//>integer
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	int Year = ml_integer_value(Args[0]);
	int Month = ml_integer_value(Args[1]);
	if (Month < 1 || Month > 12) return ml_error("ValueError", "Invalid month");
	if (Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0)) {
		static int Days[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		return ml_integer(Days[Month - 1]);
	} else {
		static int Days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		return ml_integer(Days[Month - 1]);
	}
}

#ifdef ML_TIMEZONES

typedef struct {
	ml_type_t *Type;
	const char *Id;
	timelib_tzinfo *Info;
} ml_time_zone_t;

extern ml_type_t MLTimeZoneT[];

static ml_value_t *ml_time_zone(const char *Id) {
	if (!timelib_timezone_id_is_valid(Id, timelib_builtin_db())) {
		return ml_error("TimeZoneError", "Time zone %s not found", Id);
	}
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLTimeZoneT->Exports, Id);
	if (!Slot[0]) {
		ml_time_zone_t *TimeZone = new(ml_time_zone_t);
		TimeZone->Type = MLTimeZoneT;
		TimeZone->Id = Id;
		Slot[0] = (ml_value_t *)TimeZone;
	}
	return Slot[0];
}

static ml_value_t *ml_time_zone_parse(const char *Id, int Length) {
	return ml_time_zone(Id);
}

ml_value_t *ml_parser_escape_time_zone(ml_parser_t *Parser) {
	const char *Next = ml_parser_clear(Parser);
	char Quote = *Next++;
	const char *End = Next;
	while (End[0] != Quote) {
		if (!End[0]) {
			ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
			break;
		}
		++End;
	}
	int Length = End - Next;
	char *Raw = snew(Length + 1);
	memcpy(Raw, Next, Length);
	Raw[Length] = 0;
	ml_parser_input(Parser, End + 1, 0);
	ml_value_t *Value = ml_time_zone(Raw);
	if (ml_is_error(Value)) return Value;
	mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
	ValueExpr->compile = ml_value_expr_compile;
	ml_source_t Source = ml_parser_position(Parser);
	ValueExpr->Source = Source.Name;
	ValueExpr->StartLine = ValueExpr->EndLine = Source.Line;
	ValueExpr->Value = Value;
	return ml_expr_value((mlc_expr_t *)ValueExpr);
}

static ml_value_t *ml_time_zone_deref(ml_time_zone_t *TimeZone) {
	if (!TimeZone->Info) {
		int Error = 0;
		TimeZone->Info = timelib_parse_tzfile(TimeZone->Id, timelib_builtin_db(), &Error);
	}
	return (ml_value_t *)TimeZone;
}

static void ml_time_zone_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	if (Count == 0) {
		ML_RETURN(ml_time_zone(tzname[0]));
	} else {
		ml_value_t *NameArg = ml_deref(Args[0]);
		if (!ml_is(NameArg, MLStringT)) ML_ERROR("TypeError", "Expected string not %s for arg 1", ml_typeof(NameArg)->Name);
		const char *Name = ml_string_value(NameArg);
		ML_RETURN(ml_time_zone(Name));
	}
}

ML_TYPE(MLTimeZoneTypeT, (MLTypeT, MLSequenceT), "type(time::zone)",
//!internal
	.call = (void *)ml_time_zone_call
);

ML_TYPE(MLTimeZoneT, (), "time::zone",
// A time zone.
	.deref = (void *)ml_time_zone_deref
);

ML_METHOD("::", MLTimeZoneTypeT, MLStringT) {
//<Name
//>time::zone|error
// Returns the time zone identified by :mini:`Name` or an error if no time zone is found.
	const char *Name = ml_string_value(Args[1]);
	return ml_time_zone(Name);
}

typedef struct {
	ml_type_t *Type;
	const timelib_tzdb_index_entry *Entries;
	int Total, Index;
} ml_time_zone_iter_t;

ML_TYPE(MLTimeZoneIterT, (), "time::zone::iter");
//!internal

static void ML_TYPED_FN(ml_iterate, MLTimeZoneTypeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_time_zone_iter_t *Iter = new(ml_time_zone_iter_t);
	Iter->Type = MLTimeZoneIterT;
	Iter->Entries = timelib_timezone_identifiers_list(timelib_builtin_db(), &Iter->Total);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLTimeZoneIterT, ml_state_t *Caller, ml_time_zone_iter_t *Iter) {
	if (++Iter->Index >= Iter->Total) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLTimeZoneIterT, ml_state_t *Caller, ml_time_zone_iter_t *Iter) {
	if (Iter->Index >= Iter->Total) ML_ERROR("StateError", "Invalid iterator state");
	ML_RETURN(ml_string_copy(Iter->Entries[Iter->Index].id, -1));
}

static void ML_TYPED_FN(ml_iter_value, MLTimeZoneIterT, ml_state_t *Caller, ml_time_zone_iter_t *Iter) {
	if (Iter->Index >= Iter->Total) ML_ERROR("StateError", "Invalid iterator state");
	const char *Name = Iter->Entries[Iter->Index].id;
	ML_RETURN(ml_time_zone(Name));
}

ML_METHOD("append", MLStringBufferT, MLTimeZoneT) {
//<Buffer
//<TimeZone
// Appends the name of :mini:`TimeZone` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[1];
	ml_stringbuffer_write(Buffer, TimeZone->Info->name, strlen(TimeZone->Info->name));
	return MLSome;
}

ML_METHOD(MLTimeT, MLStringT, MLTimeZoneT) {
//<String
//<TimeZone
//>time
// Parses the :mini:`String` as a time in the specified time zone.
//$= time("2023-02-09T21:19:33.196413266", time::zone::"America/Chicago")
	const char *Value = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	struct tm TM = {0,};
	unsigned long NSec = 0;
	if (Length > 10) {
		const char *Rest = strptime(Value, "%FT%T", &TM);
		if (!Rest) Rest = strptime(Value, "%F %T", &TM);
		if (!Rest) return ml_error("TimeError", "Error parsing time");
		if (Rest[0] == '.') {
			++Rest;
			char *End;
			NSec = strtoul(Rest, &End, 10);
			for (int I = 9 - (End - Rest); --I >= 0;) NSec *= 10;
			Rest = End;
		}
		if (Rest[0]) return ml_error("TimeError", "Error parsing time");
	} else {
		if (!strptime(Value, "%F", &TM)) return ml_error("TimeError", "Error parsing time");
	}
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[1];
	timelib_time TL = {0,};
	TL.y = TM.tm_year + 1900;
	TL.m = TM.tm_mon + 1;
	TL.d = TM.tm_mday;
	TL.h = TM.tm_hour;
	TL.i = TM.tm_min;
	TL.s = TM.tm_sec;
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_update_ts(&TL, TimeZone->Info);
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = TL.sse;
	Time->Value->tv_nsec = NSec;
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLStringT, MLStringT, MLTimeZoneT) {
//<String
//<Format
//<TimeZone
//>time
// Parses the :mini:`String` as a time according to specified format in the specified time zone.
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[1];
	timelib_time TL = {0,};
	TL.y = TM.tm_year + 1900;
	TL.m = TM.tm_mon + 1;
	TL.d = TM.tm_mday;
	TL.h = TM.tm_hour;
	TL.i = TM.tm_min;
	TL.s = TM.tm_sec;
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_update_ts(&TL, TimeZone->Info);
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = TL.sse;
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLIntegerT, MLTimeZoneT) {
//<Year
//<Month
//<Day
//<Hour
//<Minute
//<Second
//<TimeZone
//>time
// Returns the time specified by the provided components in the specified time zone.
	timelib_time TL = {0,};
	TL.y = ml_integer_value(Args[0]);
	TL.m = ml_integer_value(Args[1]);
	TL.d = ml_integer_value(Args[2]);
	TL.h = ml_integer_value(Args[3]);
	TL.i = ml_integer_value(Args[4]);
	TL.s = ml_integer_value(Args[5]);
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[6];
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_update_ts(&TL, TimeZone->Info);
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = TL.sse;
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLIntegerT, MLIntegerT, MLIntegerT, MLTimeZoneT) {
//<Year
//<Month
//<Day
//<TimeZone
//>time
// Returns the time specified by the provided components in the specified time zone.
	timelib_time TL = {0,};
	TL.y = ml_integer_value(Args[0]);
	TL.m = ml_integer_value(Args[1]);
	TL.d = ml_integer_value(Args[2]);
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[3];
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_update_ts(&TL, TimeZone->Info);
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = TL.sse;
	return (ml_value_t *)Time;
}

ML_METHODV("with", MLTimeT, MLTimeZoneT, MLNamesT) {
//<Time
//<TimeZone
//<Component,Value
//>time
// Returns :mini:`Time` with the the specified components updated in the specified time zone.
	ML_NAMES_CHECK_ARG_COUNT(2);
	for (int I = 3; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLIntegerT);
	ml_time_t *Time = (ml_time_t *)Args[0];
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[1];
	timelib_time TL = {0,};
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_unixtime2local(&TL, Time->Value->tv_sec);
	ml_value_t **Arg = Args + 3;
	ML_NAMES_FOREACH(Args[2], Iter) {
		const char *Part = ml_string_value(Iter->Value);
		if (!strcmp(Part, "year")) {
			TL.y = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "month")) {
			TL.m = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "day")) {
			TL.d = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "hour")) {
			TL.h = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "minute")) {
			TL.i = ml_integer_value(*Arg++);
		} else if (!strcmp(Part, "second")) {
			TL.s = ml_integer_value(*Arg++);
		} else {
			return ml_error("ValueError", "Unknown time component %s", Part);
		}
	}
	timelib_update_ts(&TL, TimeZone->Info);
	Time = new(ml_time_t);
	Time->Type = MLTimeT;
	Time->Value->tv_sec = TL.sse;
	return (ml_value_t *)Time;
}

ML_METHOD("append", MLStringBufferT, MLTimeT, MLTimeZoneT) {
//<Buffer
//<Time
//<TimeZone
//>string
// Formats :mini:`Time` as a time in :mini:`TimeZone`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[2];
	timelib_time TL = {0,};
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_unixtime2local(&TL, Time->Value->tv_sec);
	struct tm TM = {0,};
	TM.tm_year = TL.y - 1900;
	TM.tm_mon = TL.m - 1;
	TM.tm_mday = TL.d;
	TM.tm_hour = TL.h;
	TM.tm_min = TL.i;
	TM.tm_sec = TL.s;
	TM.tm_yday = timelib_day_of_year(TL.y, TL.m, TL.d);
	TM.tm_wday = timelib_day_of_week(TL.y, TL.m, TL.d);
	char Temp[60];
	size_t Length;
	unsigned long NSec = Time->Value->tv_nsec / 1000;
	if (NSec) {
		int Width = 6;
		while (NSec % 10 == 0) {
			--Width;
			NSec /= 10;
		}
		Length = strftime(Temp, 40, "%FT%T", &TM);
		Length += sprintf(Temp + Length, ".%0*lu", Width, NSec);
	} else {
		Length = strftime(Temp, 60, "%FT%T", &TM);
	}
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLTimeT, MLStringT, MLTimeZoneT) {
//<Buffer
//<Time
//<Format
//<TimeZone
//>string
// Formats :mini:`Time` as a time in :mini:`TimeZone` according to the specified format.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_time_t *Time = (ml_time_t *)Args[1];
	const char *Format = ml_string_value(Args[2]);
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[3];
	timelib_time TL = {0,};
	timelib_set_timezone(&TL, TimeZone->Info);
	timelib_unixtime2local(&TL, Time->Value->tv_sec);
	struct tm TM = {0,};
	TM.tm_year = TL.y - 1900;
	TM.tm_mon = TL.m - 1;
	TM.tm_mday = TL.d;
	TM.tm_hour = TL.h;
	TM.tm_min = TL.i;
	TM.tm_sec = TL.s;
	TM.tm_yday = timelib_day_of_year(TL.y, TL.m, TL.d);
	TM.tm_wday = timelib_day_of_week(TL.y, TL.m, TL.d);
	char Temp[120];
	size_t Length = strftime(Temp, 120, Format, &TM);
	ml_stringbuffer_write(Buffer, Temp, Length);
	return MLSome;
}

#define ML_TIME_PART_WITH_ZONE(NAME, DESC, EXPR) \
\
ML_METHOD(NAME, MLTimeT, MLTimeZoneT) { \
/*<Time
//<TimeZone
//>integer
// Returns the DESC from :mini:`Time` in :mini:`TimeZone`.
*/ \
	ml_time_t *Time = (ml_time_t *)Args[0]; \
	ml_time_zone_t *TimeZone = (ml_time_zone_t *)Args[1]; \
	timelib_time TL = {0,}; \
	timelib_set_timezone(&TL, TimeZone->Info); \
	timelib_unixtime2local(&TL, Time->Value->tv_sec); \
	return EXPR; \
}

ML_TIME_PART_WITH_ZONE("year", year, ml_integer(TL.y))
ML_TIME_PART_WITH_ZONE("month", month, ml_enum_value(MLTimeMonthT, TL.m))
ML_TIME_PART_WITH_ZONE("day", date, ml_integer(TL.d))
ML_TIME_PART_WITH_ZONE("yday", number of days from the start of the year, ml_integer(timelib_day_of_year(TL.y, TL.m, TL.d) + 1))
ML_TIME_PART_WITH_ZONE("wday", day of the week, ml_enum_value(MLTimeDayT, timelib_iso_day_of_week(TL.y, TL.m, TL.d)))
ML_TIME_PART_WITH_ZONE("hour", hour, ml_integer(TL.h))
ML_TIME_PART_WITH_ZONE("minute", minute, ml_integer(TL.i))
ML_TIME_PART_WITH_ZONE("second", second, ml_integer(TL.s))

#endif

#ifdef ML_CBOR

#include "ml_cbor.h"

static void ML_TYPED_FN(ml_cbor_write, MLTimeT, ml_cbor_writer_t *Writer, ml_time_t *Time) {
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	char Buffer[60];
	char *End = Buffer + strftime(Buffer, 50, "%FT%T", &TM);
	unsigned long NSec = Time->Value->tv_nsec;
	*End++ = '.';
	*End++ = '0' + (NSec / 100000000) % 10;
	*End++ = '0' + (NSec / 10000000) % 10;
	*End++ = '0' + (NSec / 1000000) % 10;
	*End++ = '0' + (NSec / 100000) % 10;
	*End++ = '0' + (NSec / 10000) % 10;
	*End++ = '0' + (NSec / 1000) % 10;
	*End++ = 'Z';
	*End = 0;
	ml_cbor_write_tag(Writer, 0);
	size_t Length = End - Buffer;
	ml_cbor_write_string(Writer, Length);
	ml_cbor_write_raw(Writer, (const unsigned char *)Buffer, Length);
}

static ml_value_t *ml_cbor_read_time_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (ml_is(Value, MLNumberT)) {
		ml_time_t *Time = new(ml_time_t);
		Time->Type = MLTimeT;
		Time->Value->tv_sec = ml_integer_value(Value);
		return (ml_value_t *)Time;
	} else if (ml_is(Value, MLStringT)) {
		const char *String = ml_string_value(Value);
		ml_time_t *Time = new(ml_time_t);
		Time->Type = MLTimeT;
		struct tm TM = {0,};
		int Offset = 0;
		const char *Rest = strptime(String, "%FT%T", &TM);
		if (!Rest) return ml_error("TimeError", "Error parsing time");
		if (Rest[0] == '.') {
			++Rest;
			char *End;
			unsigned long NSec = strtoul(Rest, &End, 10);
			for (int I = 9 - (End - Rest); --I >= 0;) NSec *= 10;
			Time->Value->tv_nsec = NSec;
			Rest = End;
		}
		if (Rest[0] == '+' || Rest[0] == '-') {
			const char *Next = Rest + 1;
			if (!isdigit(Next[0]) || !isdigit(Next[1])) return ml_error("TimeError", "Error parsing time");
			Offset = 3600 * (10 * (Next[0] - '0') + (Next[1] - '0'));
			Next += 2;
			if (Next[0] == ':') ++Next;
			if (isdigit(Next[0]) && isdigit(Next[1])) {
				Offset += 60 * (10 * (Next[0] - '0') + (Next[1] - '0'));
			}
			if (Rest[0] == '-') Offset = -Offset;
		}
		Time->Value->tv_sec = timegm(&TM) - Offset;
		return (ml_value_t *)Time;
	} else {
		return ml_error("TagError", "Time requires string / number");
	}
}

#endif

void ml_time_init(stringmap_t *Globals) {
#include "ml_time_init.c"
	stringmap_insert(MLTimeT->Exports, "part", ml_module("part",
		"Second", ml_integer(1),
		"Minute", ml_integer(60),
		"Hour", ml_integer(60 * 60),
		"Day", ml_integer(60 * 60 * 24),
	NULL));
	stringmap_insert(MLTimeT->Exports, "mdays", MLTimeMdays);
	stringmap_insert(MLTimeT->Exports, "day", MLTimeDayT);
	stringmap_insert(MLTimeT->Exports, "month", MLTimeMonthT);
	ml_method_by_value(MLTimeT->Constructor, NULL, ml_identity, MLTimeT, NULL);
	if (Globals) stringmap_insert(Globals, "time", MLTimeT);
	//ml_string_fn_register("T", ml_time_parse);
	ml_parser_add_escape(NULL, "T", ml_parser_escape_time);
#ifdef ML_TIMEZONES
	stringmap_insert(MLTimeT->Exports, "zone", MLTimeZoneT);
	MLTimeZoneT->Type = MLTimeZoneTypeT;
	//ml_string_fn_register("TZ", ml_time_zone_parse);
	ml_parser_add_escape(NULL, "TZ", ml_parser_escape_time_zone);
#endif
	ml_externals_default_add("time", MLTimeT);
#ifdef ML_CBOR
	ml_cbor_default_tag(ML_CBOR_TAG_TIME_STRING, ml_cbor_read_time_fn);
	ml_cbor_default_tag(ML_CBOR_TAG_TIME_EPOCH, ml_cbor_read_time_fn);
#endif
}
