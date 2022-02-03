#ifndef ML_POLY_H
#define ML_POLY_H

#include "stringmap.h"
#include "ml_types.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	int Variable, Degree;
} ml_factor_t;

typedef struct {
	int Degree, Count;
	ml_factor_t Factors[];
} ml_factors_t;

typedef struct {
	const ml_factors_t *Factors;
	double Coeff;
} ml_term_t;

typedef struct {
	ml_type_t *Type;
	int Count;
	ml_term_t Terms[];
} ml_polynomial_t;

const char *ml_polynomial_name(int Index);

void ml_polynomial_init(stringmap_t *Globals);

#ifdef	__cplusplus
}
#endif

#endif
