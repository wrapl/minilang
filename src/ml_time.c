#include "ml_time.h"
#include "ml_macros.h"
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifdef ML_CBOR
#include "ml_cbor.h"
#endif

typedef struct {
	const ml_type_t *Type;
	struct timespec Value[1];
} ml_time_t;

static long ml_time_hash(ml_time_t *Time, ml_hash_chain_t *Chain) {
	return (long)Time->Value->tv_sec;
}

ML_TYPE(MLTimeT, (), "time",
// A time in UTC with nanosecond resolution.
	.hash = (void *)ml_time_hash
);

void ml_time_value(ml_value_t *Value, struct timespec *Time) {
	Time[0] = ((ml_time_t *)Value)->Value[0];
}

ML_METHOD(MLTimeT) {
//>time
// Returns the current UTC time.
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
		Time->Value->tv_sec = timelocal(&TM);
	} else {
		Time->Value->tv_sec = timegm(&TM) - Offset;
	}
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLStringT) {
//<String
//>time
// Parses the :mini:`String` as a time according to ISO 8601.
	return ml_time_parse(ml_string_value(Args[0]), ml_string_length(Args[0]));
}

ML_METHOD(MLTimeT, MLStringT, MLStringT) {
//<String
//<Format
//>time
// Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	Time->Value->tv_sec = timelocal(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLStringT, MLStringT, MLBooleanT) {
//<String
//<Format
//<UTC
//>time
// Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time unless UTC is :mini:`true`.
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	if (ml_boolean_value(Args[2])) {
		Time->Value->tv_sec = timegm(&TM);
	} else {
		Time->Value->tv_sec = timelocal(&TM);
	}
	return (ml_value_t *)Time;
}

ML_METHOD("nsec", MLTimeT) {
//<Time
//>integer
// Returns the nanoseconds component of :mini:`Time`.
	ml_time_t *Time = (ml_time_t *)Args[0];
	return ml_integer(Time->Value->tv_nsec);
}

ML_METHOD(MLStringT, MLTimeT) {
//<Time
//>string
// Formats :mini:`Time` as a local time.
	ml_time_t *Time = (ml_time_t *)Args[0];
	struct tm TM = {0,};
	localtime_r(&Time->Value->tv_sec, &TM);
	char Buffer[30];
	size_t Length = strftime(Buffer, 30, "%F %T", &TM);
	return ml_string(GC_strdup(Buffer), Length);
}

ML_METHOD(MLStringT, MLTimeT, MLNilT) {
//<Time
//<TimeZone
//>string
// Formats :mini:`Time` as a UTC time according to ISO 8601.
	ml_time_t *Time = (ml_time_t *)Args[0];
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	char Buffer[50];
	size_t Length;
	unsigned long NSec = Time->Value->tv_nsec;
	if (NSec) {
		int Width = 9;
		while (NSec % 10 == 0) {
			--Width;
			NSec /= 10;
		}
		Length = strftime(Buffer, 40, "%FT%T", &TM);
		Length += sprintf(Buffer + Length, ".%0*luZ", Width, NSec);
	} else {
		Length = strftime(Buffer, 60, "%FT%TZ", &TM);
	}
	return ml_string(GC_strdup(Buffer), Length);
}

ML_METHOD(MLStringT, MLTimeT, MLStringT) {
//<Time
//<Format
//>string
// Formats :mini:`Time` as a local time according to the specified format.
	ml_time_t *Time = (ml_time_t *)Args[0];
	const char *Format = ml_string_value(Args[1]);
	struct tm TM = {0,};
	localtime_r(&Time->Value->tv_sec, &TM);
	char Buffer[120];
	size_t Length = strftime(Buffer, 120, Format, &TM);
	return ml_string(GC_strdup(Buffer), Length);
}

ML_METHOD(MLStringT, MLTimeT, MLStringT, MLNilT) {
//<Time
//<Format
//<TimeZone
//>string
// Formats :mini:`Time` as a UTC time according to the specified format.
	ml_time_t *Time = (ml_time_t *)Args[0];
	const char *Format = ml_string_value(Args[1]);
	struct tm TM = {0,};
	if (ml_boolean_value(Args[2])) {
		gmtime_r(&Time->Value->tv_sec, &TM);
	} else {
		localtime_r(&Time->Value->tv_sec, &TM);
	}
	char Buffer[120];
	size_t Length = strftime(Buffer, 120, Format, &TM);
	return ml_string(GC_strdup(Buffer), Length);
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
	ml_time_t *TimeA = (ml_time_t *)Args[0];
	ml_time_t *TimeB = (ml_time_t *)Args[1];
	double Sec = difftime(TimeA->Value->tv_sec, TimeB->Value->tv_sec);
	double NSec = ((double)TimeA->Value->tv_nsec - (double)TimeB->Value->tv_nsec) / 1000000000.0;
	return ml_real(Sec + NSec);
}

ML_METHOD("+", MLTimeT, MLNumberT) {
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

#ifdef ML_CBOR

#include "ml_cbor.h"

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLTimeT, ml_time_t *Time, void *Data, ml_cbor_write_fn WriteFn) {
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
	ml_cbor_write_tag(Data, WriteFn, 0);
	size_t Length = End - Buffer;
	ml_cbor_write_string(Data, WriteFn, Length);
	WriteFn(Data, (const unsigned char *)Buffer, Length);
	return NULL;
}

static ml_value_t *ml_cbor_read_time_fn(void *Data, int Count, ml_value_t **Args) {
	if (ml_is(Args[0], MLNumberT)) {
		ml_time_t *Time = new(ml_time_t);
		Time->Type = MLTimeT;
		Time->Value->tv_sec = ml_integer_value(Args[0]);
		return (ml_value_t *)Time;
	} else if (ml_is(Args[0], MLStringT)) {
		const char *Value = ml_string_value(Args[0]);
		ml_time_t *Time = new(ml_time_t);
		Time->Type = MLTimeT;
		struct tm TM = {0,};
		int Offset = 0;
		const char *Rest = strptime(Value, "%FT%T", &TM);
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
	stringmap_insert(MLTimeT->Exports, "second", ml_integer(1));
	stringmap_insert(MLTimeT->Exports, "minute", ml_integer(60));
	stringmap_insert(MLTimeT->Exports, "hour", ml_integer(3600));
	stringmap_insert(MLTimeT->Exports, "day", ml_integer(86400));
	stringmap_insert(MLTimeT->Exports, "week", ml_integer(604800));
	stringmap_insert(MLTimeT->Exports, "year", ml_integer(31557600));
	if (Globals) stringmap_insert(Globals, "time", MLTimeT);
	ml_string_fn_register("T", ml_time_parse);
#ifdef ML_CBOR
	ml_cbor_default_tag(0, NULL, ml_cbor_read_time_fn);
	ml_cbor_default_tag(1, NULL, ml_cbor_read_time_fn);
#endif
}
