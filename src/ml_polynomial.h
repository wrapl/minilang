#ifndef ML_POLY_H
#define ML_POLY_H

#include "stringmap.h"
#include "ml_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int Variable, Degree;
} ml_factor_t;

typedef struct {
	int Degree, Count;
	ml_factor_t Factors[];
} ml_factors_t;

#ifdef ML_COMPLEX
#include <complex.h>
#undef I

typedef complex double ml_coeff_t;

#else

typedef double ml_coeff_t;

#endif

typedef struct {
	const ml_factors_t *Factors;
	ml_coeff_t Coeff;
} ml_term_t;

typedef struct {
	ml_type_t *Type;
	int Count;
	ml_term_t Terms[];
} ml_polynomial_t;

extern ml_type_t MLPolynomialT[];

typedef struct {
	ml_type_t *Type;
	ml_polynomial_t *A, *B;
} ml_polynomial_rational_t;

extern ml_type_t MLPolynomialRationalT[];

const char *ml_polynomial_name(int Index);

void ml_polynomial_write(ml_stringbuffer_t *Buffer, ml_polynomial_t *Poly);

void ml_polynomial_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif
