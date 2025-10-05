#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#undef ML_CATEGORY
#define ML_CATEGORY "boolean"

static long ml_boolean_hash(ml_boolean_t *Boolean, ml_hash_chain_t *Chain) {
	return (long)Boolean;
}

ML_TYPE(MLBooleanT, (), "boolean",
//!boolean
// A boolean value (either :mini:`true` or :mini:`false`).
	.hash = (void *)ml_boolean_hash
);

int ml_boolean_value(const ml_value_t *Value) {
	return ((ml_boolean_t *)Value)->Value;
}

ml_boolean_t MLFalse[1] = {{MLBooleanT, "false", 0}};
ml_boolean_t MLTrue[1] = {{MLBooleanT, "true", 1}};

static ml_value_t *MLBooleans[2] = {
	[0] = (ml_value_t *)MLFalse,
	[1] = (ml_value_t *)MLTrue
};

ml_value_t *ml_boolean(int Value) {
	return Value ? (ml_value_t *)MLTrue : (ml_value_t *)MLFalse;
}

static int ML_TYPED_FN(ml_value_is_constant, MLBooleanT, ml_value_t *Value) {
	return 1;
}

ML_METHOD(MLBooleanT, MLStringT) {
//!boolean
//<String
//>boolean | error
// Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).
// Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).
// Otherwise returns an error.
	const char *Name = ml_string_value(Args[0]);
	if (!strcasecmp(Name, "true")) return (ml_value_t *)MLTrue;
	if (!strcasecmp(Name, "false")) return (ml_value_t *)MLFalse;
	return ml_error("ValueError", "Invalid boolean: %s", Name);
}

ML_METHOD("-", MLBooleanT) {
//!boolean
//<Bool
//>boolean
// Returns the logical inverse of :mini:`Bool`
	return MLBooleans[1 - ml_boolean_value(Args[0])];
}

ML_METHODV("/\\", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical and of :mini:`Bool/1` and :mini:`Bool/2`.
//$= true /\ true
//$= true /\ false
//$= false /\ true
//$= false /\ false
	int Result = ml_boolean_value(Args[0]);
	for (int I = 1; I < Count; ++I) Result &= ml_boolean_value(Args[I]);
	return MLBooleans[Result];
}

ML_METHODV("\\/", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical or of :mini:`Bool/1` and :mini:`Bool/2`.
//$= true \/ true
//$= true \/ false
//$= false \/ true
//$= false \/ false
	int Result = ml_boolean_value(Args[0]);
	for (int I = 1; I < Count; ++I) Result |= ml_boolean_value(Args[I]);
	return MLBooleans[Result];
}

ML_METHOD("><", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical xor of :mini:`Bool/1` and :mini:`Bool/2`.
//$= true >< true
//$= true >< false
//$= false >< true
//$= false >< false
	int Result = ml_boolean_value(Args[0]) != ml_boolean_value(Args[1]);
	return MLBooleans[Result];
}

ML_METHOD("<>", MLBooleanT, MLBooleanT) {
//<Bool/1
//<Bool/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Bool/1` is less than, equal to or greater than :mini:`Bool/2`. :mini:`true` is considered greater than :mini:`false`.
	ml_boolean_t *BooleanA = (ml_boolean_t *)Args[0];
	ml_boolean_t *BooleanB = (ml_boolean_t *)Args[1];
	return ml_integer(BooleanA->Value - BooleanB->Value);
}

#define ml_comp_method_boolean_boolean(NAME, SYMBOL) \
ML_METHOD(#NAME, MLBooleanT, MLBooleanT) { \
/*>boolean|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
//$= true NAME true
//$= true NAME false
//$= false NAME true
//$= false NAME false
*/\
	ml_boolean_t *BooleanA = (ml_boolean_t *)Args[0]; \
	ml_boolean_t *BooleanB = (ml_boolean_t *)Args[1]; \
	return BooleanA->Value SYMBOL BooleanB->Value ? Args[1] : MLNil; \
}

ml_comp_method_boolean_boolean(=, ==);
ml_comp_method_boolean_boolean(!=, !=);
ml_comp_method_boolean_boolean(<, <);
ml_comp_method_boolean_boolean(>, >);
ml_comp_method_boolean_boolean(<=, <=);
ml_comp_method_boolean_boolean(>=, >=);

ML_FUNCTION(RandomBoolean) {
//@boolean::random
//<P?:number
//>boolean
// Returns a random boolean that has probability :mini:`P` of being :mini:`true`. If omitted, :mini:`P` defaults to :mini:`0.5`.
	int Threshold;
	if (Count == 1) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		Threshold = RAND_MAX * ml_real_value(Args[0]);
	} else {
		Threshold = RAND_MAX / 2;
	}
	return (ml_value_t *)(rand() > Threshold ? MLFalse : MLTrue);
}

void ml_boolean_init() {
#include "ml_boolean_init.c"
	stringmap_insert(MLBooleanT->Exports, "random", RandomBoolean);
}
