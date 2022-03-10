#ifndef ML_MATH_H
#define ML_MATH_H

#include "minilang.h"

extern ml_value_t *AcosMethod;
extern ml_value_t *AsinMethod;
extern ml_value_t *AtanMethod;
extern ml_value_t *CeilMethod;
extern ml_value_t *CosMethod;
extern ml_value_t *CoshMethod;
extern ml_value_t *ExpMethod;
extern ml_value_t *AbsMethod;
extern ml_value_t *FloorMethod;
extern ml_value_t *LogMethod;
extern ml_value_t *Log10Method;
extern ml_value_t *LogitMethod;
extern ml_value_t *SinMethod;
extern ml_value_t *SinhMethod;
extern ml_value_t *SqrtMethod;
extern ml_value_t *SquareMethod;
extern ml_value_t *TanMethod;
extern ml_value_t *TanhMethod;
extern ml_value_t *ErfMethod;
extern ml_value_t *ErfcMethod;
extern ml_value_t *GammaMethod;
extern ml_value_t *AcoshMethod;
extern ml_value_t *AsinhMethod;
extern ml_value_t *AtanhMethod;
extern ml_value_t *CbrtMethod;
extern ml_value_t *Expm1Method;
extern ml_value_t *Log1pMethod;
extern ml_value_t *RoundMethod;
extern ml_value_t *ArgMethod;
extern ml_value_t *ConjMethod;

double logit(double X);

void ml_math_init(stringmap_t *Globals);

#endif
