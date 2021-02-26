#include "ml_time.h"
#include "ml_macros.h"
#include <string.h>

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
	.hash = (void *)ml_time_hash
);

void ml_time_value(ml_value_t *Value, struct timespec *Time) {
	Time[0] = ((ml_time_t *)Value)->Value[0];
}

ML_METHOD(MLTimeT) {
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
	if (Length > 10) {
		const char *Rest = strptime(Value, "%F %T", &TM);
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
			// Do something with timezones
		}
	} else {
		if (!strptime(Value, "%F", &TM)) return ml_error("TimeError", "Error parsing time");
	}
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD(MLTimeT, MLStringT) {
	return ml_time_parse(ml_string_value(Args[0]), ml_string_length(Args[0]));
}

ML_METHOD(MLTimeT, MLStringT, MLStringT) {
	ml_time_t *Time = new(ml_time_t);
	Time->Type = MLTimeT;
	struct tm TM = {0,};
	if (!strptime(ml_string_value(Args[0]), ml_string_value(Args[1]), &TM)) {
		return ml_error("TimeError", "Error parsing time");
	}
	Time->Value->tv_sec = timegm(&TM);
	return (ml_value_t *)Time;
}

ML_METHOD("nsec", MLTimeT) {
	ml_time_t *Time = (ml_time_t *)Args[0];
	return ml_integer(Time->Value->tv_nsec);
}

ML_METHOD(MLStringT, MLTimeT) {
	ml_time_t *Time = (ml_time_t *)Args[0];
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	size_t Length = strftime(NULL, -1, "%F %T", &TM);
	char *String;
	unsigned long NSec = Time->Value->tv_nsec;
	if (NSec) {
		int Width = 9;
		while (NSec % 10 == 0) {
			--Width;
			NSec /= 10;
		}
		Length += Width + 1;
		String = snew(Length + 1);
		char *End = String + strftime(String, Length + 1, "%F %T", &TM);
		sprintf(End, ".%0*lu", Width, NSec);
	} else {
		strftime(String = snew(Length + 1), Length + 1, "%F %T", &TM);
	}
	return ml_string(String, Length);
}

ML_METHOD(MLStringT, MLTimeT, MLStringT) {
	ml_time_t *Time = (ml_time_t *)Args[0];
	const char *Format = ml_string_value(Args[1]);
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	size_t Length = strftime(NULL, 0, Format, &TM);
	char *String = snew(Length + 1);
	strftime(NULL, Length + 1, Format, &TM);
	return ml_string(String, Length);
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
	double NSec = (TimeA->Value->tv_nsec / 1000000000.0) - (TimeB->Value->tv_nsec / 1000000000.0);
	return ml_real(Sec + NSec);
}

#ifdef ML_CBOR

#include "ml_cbor.h"

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLTimeT, ml_time_t *Time, void *Data, ml_cbor_write_fn WriteFn) {
	struct tm TM = {0,};
	gmtime_r(&Time->Value->tv_sec, &TM);
	size_t Length = strftime(NULL, -1, "%FT%T", &TM);
	char *String;
	unsigned long NSec = Time->Value->tv_nsec;
	int Width = 9;
	while (NSec % 10 == 0) {
		--Width;
		NSec /= 10;
	}
	Length += Width + 1;
	String = snew(Length + 1);
	char *End = String + strftime(String, Length + 1, "%FT%T", &TM);
	sprintf(End, ".%0*lu", Width, NSec);
	ml_cbor_write_tag(Data, WriteFn, 0);
	ml_cbor_write_string(Data, WriteFn, Length);
	WriteFn(Data, (const unsigned char *)String, Length);
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
			// Do something with timezones
		}
		Time->Value->tv_sec = timegm(&TM);
		return (ml_value_t *)Time;
	} else {
		return ml_error("TagError", "Time requires string / number");
	}
}

#endif

void ml_time_init(stringmap_t *Globals) {
#include "ml_time_init.c"
	if (Globals) stringmap_insert(Globals, "time", MLTimeT);
	ml_string_fn_register("T", ml_time_parse);
#ifdef ML_CBOR
	ml_cbor_default_tag(0, NULL, ml_cbor_read_time_fn);
	ml_cbor_default_tag(1, NULL, ml_cbor_read_time_fn);
#endif
}
