#include "ml_polynomial.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include "ml_sequence.h"

//#define ML_POLY_DEBUG

#ifdef ML_COMPLEX
#define abs cabs
#define ml_coeff ml_complex
#define ml_coeff_value ml_complex_value
#else
#define abs fabs
#define ml_coeff ml_real
#define ml_coeff_value ml_real_value
#endif

static stringmap_t Variables[1] = {STRINGMAP_INIT};
static const char **Names = 0;

const char *ml_polynomial_name(int Index) {
	if (Index < 0 || Index >= Variables->Size) return NULL;
	return Names[Index];
}

static ml_factors_t Constant[1] = {{0, 0}};

static int ml_factors_cmp(const ml_factors_t *A, const ml_factors_t *B) {
	if (A->Degree < B->Degree) return -1;
	if (A->Degree > B->Degree) return 1;
	const ml_factor_t *FA = A->Factors, *FB = B->Factors;
	if (A->Count < B->Count) {
		for (int I = 0; I < A->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return -1;
	} else if (A->Count > B->Count) {
		for (int I = 0; I < B->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return 1;
	} else {
		for (int I = 0; I < A->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return 0;
	}
}

static const ml_factors_t *ml_factors_mul(const ml_factors_t *A, const ml_factors_t *B) {
	if (A->Count == 0) return B;
	if (B->Count == 0) return A;
	ml_factors_t *C = xnew(ml_factors_t, A->Count + B->Count, ml_factor_t);
	const ml_factor_t *FA = A->Factors, *FB = B->Factors;
	ml_factor_t *FC = C->Factors;
	int CA = A->Count, CB = B->Count;
	int Degree = 0;
	while (CA && CB) {
		if (FA->Variable < FB->Variable) {
			Degree += FA->Degree;
			*FC++ = *FA++;
			--CA;
		} else if (FA->Variable > FB->Variable) {
			Degree += FB->Degree;
			*FC++ = *FB++;
			--CB;
		} else {
			if ((FC->Degree = FA->Degree + FB->Degree)) {
				Degree += FC->Degree;
				FC->Variable = FA->Variable;
				++FC;
			}
			++FA; ++FB;
			--CA; --CB;
		}
	}
	while (CA) {
		Degree += FA->Degree;
		*FC++ = *FA++;
		--CA;
	}
	while (CB) {
		Degree += FB->Degree;
		*FC++ = *FB++;
		--CB;
	}
	C->Count = FC - C->Factors;
	C->Degree = Degree;
	return C;
}

static ml_value_t *ml_polynomial_value(ml_polynomial_t *P) {
	if (P->Count == 0) return ml_real(0);
	if (P->Count == 1 && P->Terms->Factors->Count == 0) return ml_real(P->Terms->Coeff);
	return (ml_value_t *)P;
}

void ml_polynomial_write(ml_stringbuffer_t *Buffer, ml_polynomial_t *Poly) {
	const ml_term_t *Terms = Poly->Terms;
	for (int I = 0; I < Poly->Count; ++I) {
		const ml_term_t *Term = Terms + I;
		ml_coeff_t Coeff = Term->Coeff;
		if (Term->Factors->Count == 0) {
#ifdef ML_COMPLEX
			double Real = creal(Coeff), Imag = cimag(Coeff);
			if (fabs(Real) > DBL_EPSILON) {
				if (I && Real < 0) {
					ml_stringbuffer_printf(Buffer, " - %g", -Real);
				} else if (I) {
					ml_stringbuffer_printf(Buffer, " + %g", Real);
				} else {
					ml_stringbuffer_printf(Buffer, "%g", Real);
				}
				if (Imag < -DBL_EPSILON) {
					ml_stringbuffer_printf(Buffer, " - %gi", -Imag);
				} else if (Imag > DBL_EPSILON) {
					ml_stringbuffer_printf(Buffer, " + %gi", Imag);
				}
			} else {
				if (I && Imag < 0) {
					ml_stringbuffer_printf(Buffer, " - %gi", -Imag);
				} else if (I) {
					ml_stringbuffer_printf(Buffer, " + %gi", Imag);
				} else {
					ml_stringbuffer_printf(Buffer, "%gi", Imag);
				}
			}
#else
			if (I && Coeff < 0) {
				ml_stringbuffer_printf(Buffer, " - %g", -Coeff);
			} else if (I) {
				ml_stringbuffer_printf(Buffer, " + %g", Coeff);
			} else {
				ml_stringbuffer_printf(Buffer, "%g", Coeff);
			}
#endif
		} else {
			if (abs(Coeff - 1) < DBL_EPSILON) {
				if (I) {
					ml_stringbuffer_write(Buffer, " + ", 3);
				}
			} else if (abs(Coeff + 1) < DBL_EPSILON) {
				if (I) {
					ml_stringbuffer_write(Buffer, " - ", 3);
				} else {
					ml_stringbuffer_write(Buffer, "-", 1);
				}
			} else {
#ifdef ML_COMPLEX
				double Real = creal(Coeff), Imag = cimag(Coeff);
				if (fabs(Real) > DBL_EPSILON) {
					if (fabs(Imag) > DBL_EPSILON) {
						if (I && Real < 0) {
							ml_stringbuffer_printf(Buffer, " - (%g", -Real);
						} else if (I) {
							ml_stringbuffer_printf(Buffer, " + (%g", Real);
						} else {
							ml_stringbuffer_printf(Buffer, "(%g", Real);
						}
						if (Imag < -DBL_EPSILON) {
							ml_stringbuffer_printf(Buffer, " - %gi)", -Imag);
						} else if (Imag > DBL_EPSILON) {
							ml_stringbuffer_printf(Buffer, " + %gi)", Imag);
						}
					} else {
						if (I && Real < 0) {
							ml_stringbuffer_printf(Buffer, " - %g", -Real);
						} else if (I) {
							ml_stringbuffer_printf(Buffer, " + %g", Real);
						} else {
							ml_stringbuffer_printf(Buffer, "%g", Real);
						}
						if (Imag < -DBL_EPSILON) {
							ml_stringbuffer_printf(Buffer, " - %gi", -Imag);
						} else if (Imag > DBL_EPSILON) {
							ml_stringbuffer_printf(Buffer, " + %gi", Imag);
						}
					}
				} else {
					if (I && Imag < 0) {
						ml_stringbuffer_printf(Buffer, " - %gi", -Imag);
					} else if (I) {
						ml_stringbuffer_printf(Buffer, " + %gi", Imag);
					} else {
						ml_stringbuffer_printf(Buffer, "%gi", Imag);
					}
				}
#else
				if (I && Coeff < 0) {
					ml_stringbuffer_printf(Buffer, " - %g", -Coeff);
				} else if (I) {
					ml_stringbuffer_printf(Buffer, " + %g", Coeff);
				} else {
					ml_stringbuffer_printf(Buffer, "%g", Coeff);
				}
#endif
			}
			const ml_factor_t *Factor = Term->Factors->Factors;
			for (int J = Term->Factors->Count; --J >= 0; ++Factor) {
				ml_stringbuffer_printf(Buffer, "%s", Names[Factor->Variable - 1]);
				if (Factor->Degree != 1) {
					char Degree[16];
					sprintf(Degree, "%d", Factor->Degree);
					static const char *Exponents[10] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
					for (char *D = Degree; *D; ++D) ml_stringbuffer_printf(Buffer, "%s", Exponents[*D - '0']);
				}
			}
		}
	}
}

typedef struct ml_substitution_t ml_substitution_t;

struct ml_substitution_t {
	int Variable, Degree;
	ml_value_t *Values[];
};

typedef struct {
	ml_state_t Base;
	ml_value_t *Args[3];
	ml_polynomial_t *P;
	ml_factors_t *F;
	inthash_t Subs[1];
	int I1, I2;
} ml_polynomial_call_state_t;

extern ml_value_t *MulMethod;
extern ml_value_t *AddMethod;

static void ml_polynomial_factor_run(ml_polynomial_call_state_t *State, ml_value_t *Value);
static void ml_polynomial_term_run(ml_polynomial_call_state_t *State, ml_value_t *Value);
static void ml_polynomial_call_term(ml_polynomial_call_state_t *State);

static void ml_polynomial_call_factor(ml_polynomial_call_state_t *State) {
	ml_polynomial_t *P = State->P;
	ml_term_t Term = P->Terms[State->I1];
	const ml_factors_t *F = Term.Factors;
	if (State->I2 > F->Count) {
		++State->I1;
		if (State->Args[0]) {
			State->Base.run = (ml_state_fn)ml_polynomial_term_run;
			return ml_call(State, AddMethod, 2, State->Args);
		} else {
			return ml_polynomial_term_run(State, State->Args[1]);
		}
	}
	ml_factors_t *F2 = State->F;
	for (int I = State->I2; I < F->Count; ++I) {
		ml_substitution_t *Sub = inthash_search(State->Subs, F->Factors[I].Variable);
		if (Sub) {
			State->I2 = I + 1;
			State->Args[2] = Sub->Values[F->Factors[I].Degree - 1];
			State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
			return ml_call(State, MulMethod, 2, State->Args + 1);
		} else if (F2) {
			F2->Factors[F2->Count++] = F->Factors[I];
			F2->Degree += F->Factors[I].Degree;
		} else {
			F2 = State->F = xnew(ml_factors_t, F->Count - I, ml_factor_t);
			F2->Factors[0] = F->Factors[I];
			F2->Degree += F->Factors[I].Degree;
			F2->Count = 1;
		}
	}
	State->I2 = F->Count + 1;
	if (F2) {
		ml_polynomial_t *Q = xnew(ml_polynomial_t, 1, ml_term_t);
		Q->Type = MLPolynomialT;
		Q->Count = 1;
		Q->Terms->Coeff = Term.Coeff;
		Q->Terms->Factors = F2;
		State->Args[2] = (ml_value_t *)Q;
		State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
		return ml_call(State, MulMethod, 2, State->Args + 1);
	} else {
		State->Args[2] = ml_real(Term.Coeff);
		State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
		return ml_call(State, MulMethod, 2, State->Args + 1);
	}
}

static void ml_polynomial_factor_run(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[1] = Value;
	return ml_polynomial_call_factor(State);
}

static void ml_polynomial_call_term(ml_polynomial_call_state_t *State) {
	ml_polynomial_t *P = State->P;
	if (State->I1 == P->Count) ML_CONTINUE(State->Base.Caller, State->Args[0]);
	ml_term_t Term = P->Terms[State->I1];
	if (!Term.Factors->Count) {
		if (State->Args[0]) {
			State->Args[1] = ml_real(Term.Coeff);
			return ml_call(State->Base.Caller, AddMethod, 2, State->Args);
		} else {
			ML_CONTINUE(State->Base.Caller, ml_real(Term.Coeff));
		}
	}
	State->F = NULL;
	const ml_factors_t *F = Term.Factors;
	for (int I = 0; I < F->Count; ++I) {
		ml_substitution_t *Sub = inthash_search(State->Subs, F->Factors[I].Variable);
		if (Sub) {
			if (I) {
				ml_factors_t *F2 = State->F = xnew(ml_factors_t, F->Count - 1, ml_factor_t);
				int Degree = 0;
				for (int J = 0; J < I; ++J) {
					F2->Factors[J] = F->Factors[J];
					Degree += F->Factors[J].Degree;
				}
				F2->Degree = Degree;
				F2->Count = I;
			}
			State->Args[1] = Sub->Values[F->Factors[I].Degree - 1];
			State->I2 = I + 1;
			return ml_polynomial_call_factor(State);
		}
	}
	++State->I1;
	ml_polynomial_t *Q = xnew(ml_polynomial_t, 1, ml_term_t);
	Q->Type = MLPolynomialT;
	Q->Count = 1;
	Q->Terms[0] = Term;
	if (State->Args[0]) {
		State->Args[1] = (ml_value_t *)Q;
		State->Base.run = (ml_state_fn)ml_polynomial_term_run;
		return ml_call(State, AddMethod, 2, State->Args);
	} else {
		State->Args[0] = (ml_value_t *)Q;
		return ml_polynomial_call_term(State);
	}
}

static void ml_polynomial_term_run(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	return ml_polynomial_call_term(State);
}

static void ml_polynomial_compute_powers(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_substitution_t *Sub = State->Subs->Values[State->I1];
	int I2 = State->I2;
	Sub->Values[I2] = Value;
	++I2;
	if (I2 < Sub->Degree) {
		State->I2 = I2;
		State->Args[0] = Sub->Values[0];
		State->Args[1] = Value;
		return ml_call(State, MulMethod, 2, State->Args);
	}
	for (int I1 = State->I1 + 1; I1 < State->Subs->Size; ++I1) {
		ml_substitution_t *Sub = State->Subs->Values[I1];
		if (Sub && Sub->Degree > 1) {
			State->I1 = I1;
			State->I2 = 1;
			State->Args[0] = Sub->Values[0];
			State->Args[1] = Sub->Values[0];
			State->Base.run = (ml_state_fn)ml_polynomial_compute_powers;
			return ml_call(State, MulMethod, 2, State->Args);
		}
	}
	State->I1 = 0;
	State->Args[0] = NULL;
	return ml_polynomial_call_term(State);
}

static void ml_polynomial_call(ml_state_t *Caller, ml_polynomial_t *P, int Count, ml_value_t **Args) {
	if (!Count) ML_RETURN(P);
	if (!P->Count) ML_RETURN(ml_real(0));
	if (ml_typeof(Args[0]) == MLNamesT) {
		ML_NAMES_CHECKX_ARG_COUNT(0);
		ml_polynomial_call_state_t *State = xnew(ml_polynomial_call_state_t, Count, ml_substitution_t);
		int I = 0;
		ML_NAMES_FOREACH(Args[0], Iter) {
			int Variable = (intptr_t)stringmap_search(Variables, ml_string_value(Iter->Value));
			if (Variable) {
				int MaxDegree = 0;
				for (int J = 0; J < P->Count; ++J) {
					const ml_factors_t *F = P->Terms[J].Factors;
					for (int K = 0; K < F->Count; ++K) {
						if (F->Factors[K].Variable == Variable) {
							if (MaxDegree < F->Factors[K].Degree) MaxDegree = F->Factors[K].Degree;
						}
					}
				}
				if (MaxDegree) {
					ml_substitution_t *Sub = xnew(ml_substitution_t, MaxDegree, ml_value_t *);
					Sub->Variable = Variable;
					Sub->Degree = MaxDegree;
					Sub->Values[0] = ml_deref(Args[I + 1]);
					inthash_insert(State->Subs, Variable, Sub);
				}
			}
			++I;
		}
		if (!State->Subs->Size) ML_RETURN(P);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->P = P;
		for (int I1 = 0; I1 < State->Subs->Size; ++I1) {
			ml_substitution_t *Sub = State->Subs->Values[I1];
			if (Sub && Sub->Degree > 1) {
				State->I1 = I1;
				State->I2 = 1;
				State->Args[0] = Sub->Values[0];
				State->Args[1] = Sub->Values[0];
				State->Base.run = (ml_state_fn)ml_polynomial_compute_powers;
				return ml_call(State, MulMethod, 2, State->Args);
			}
		}
		State->I1 = 0;
		State->Args[0] = NULL;
		return ml_polynomial_call_term(State);
	} else if (Count == 1) {
		ml_term_t *Term = P->Terms;
		if (Term->Factors->Count != 1) ML_ERROR("CallError", "Can not call multivariate polynomial without named arguments");
		int Variable = Term->Factors->Factors[0].Variable;
		int MaxDegree = Term->Factors->Factors[0].Degree;
		for (int I = 1; I < P->Count; ++I) {
			++Term;
			if (Term->Factors->Count > 1) ML_ERROR("CallError", "Can not call multivariate polynomial without named arguments");
			if (Term->Factors->Count == 1) {
				if (Term->Factors->Factors[0].Variable != Variable) ML_ERROR("CallError", "Can not call multivariate polynomial without named arguments");
				if (MaxDegree < Term->Factors->Factors[0].Degree) MaxDegree = Term->Factors->Factors[0].Degree;
			}
		}
		ml_polynomial_call_state_t *State = xnew(ml_polynomial_call_state_t, 1, ml_substitution_t);
		ml_substitution_t *Sub = xnew(ml_substitution_t, MaxDegree, ml_value_t *);
		Sub->Variable = Variable;
		Sub->Degree = MaxDegree;
		Sub->Values[0] = ml_deref(Args[0]);
		inthash_insert(State->Subs, Variable, Sub);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->P = P;
		for (int I1 = 0; I1 < State->Subs->Size; ++I1) {
			ml_substitution_t *Sub = State->Subs->Values[I1];
			if (Sub && Sub->Degree > 1) {
				State->I1 = I1;
				State->I2 = 1;
				State->Args[0] = Sub->Values[0];
				State->Args[1] = Sub->Values[0];
				State->Base.run = (ml_state_fn)ml_polynomial_compute_powers;
				return ml_call(State, MulMethod, 2, State->Args);
			}
		}
		State->I1 = 0;
		State->Args[0] = NULL;
		return ml_polynomial_call_term(State);
	} else {
		ML_ERROR("ImplementationError", "Not implemented yet");
	}
}

ML_TYPE(MLPolynomialT, (MLFunctionT), "polynomial",
// A polynomial with numeric (real or complex) coefficients.
// Calling a polynomial with named arguments returns the result of substituting the named variables with the corresponding values.
	.call = (void *)ml_polynomial_call
);

static ml_polynomial_t *ml_polynomial_const(ml_coeff_t Value) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, 1, ml_term_t);
	C->Type = MLPolynomialT;
	C->Count = 1;
	C->Terms->Coeff = Value;
	C->Terms->Factors = Constant;
	return C;
}

static ml_polynomial_t *ml_polynomial_add(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count + B->Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	while (CA && CB) {
		int Cmp = ml_factors_cmp(TA->Factors, TB->Factors);
		if (Cmp < 0) {
			*TC++ = *TB++;
			--CB;
		} else if (Cmp > 0) {
			*TC++ = *TA++;
			--CA;
		} else {
			ml_coeff_t Coeff = TA->Coeff + TB->Coeff;
			if (abs(Coeff) >= DBL_EPSILON) {
				TC->Coeff = Coeff;
				TC->Factors = TA->Factors;
				++TC;
			}
			++TA; ++TB;
			--CA; --CB;
		}
	}
	while (CA) {
		*TC++ = *TA++;
		--CA;
	}
	while (CB) {
		*TC++ = *TB++;
		--CB;
	}
	C->Count = TC - C->Terms;
	return C;
}

static ml_polynomial_t *ml_polynomial_sub(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count + B->Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	while (CA && CB) {
		int Cmp = ml_factors_cmp(TA->Factors, TB->Factors);
		if (Cmp < 0) {
			TC->Factors = TB->Factors;
			TC->Coeff = -TB->Coeff;
			++TC;
			++TB;
			--CB;
		} else if (Cmp > 0) {
			*TC++ = *TA++;
			--CA;
		} else {
			ml_coeff_t Coeff = TA->Coeff - TB->Coeff;
			if (abs(Coeff) >= DBL_EPSILON) {
				TC->Coeff = Coeff;
				TC->Factors = TA->Factors;
				++TC;
			}
			++TA; ++TB;
			--CA; --CB;
		}
	}
	while (CA) {
		*TC++ = *TA++;
		--CA;
	}
	while (CB) {
		TC->Factors = TB->Factors;
		TC->Coeff = -TB->Coeff;
		++TB;
		++TC;
		--CB;
	}
	C->Count = TC - C->Terms;
	return C;
}

static void ml_terms_sort(ml_term_t *Lo, ml_term_t *Hi) {
	ml_term_t *A = Lo, *B = Hi;
	ml_term_t P = *A, T = *B;
	while (A < B) {
		int Cmp = ml_factors_cmp(P.Factors, T.Factors);
		if (Cmp < 0) {
			*A++ = T;
			T = *A;
		} else if (Cmp > 0) {
			*B-- = T;
			T = *B;
		} else {
			P.Coeff += T.Coeff;
			T.Coeff = 0;
			*B-- = T;
			T = *B;
		}
	}
	*A = P;
	if (Lo < A - 1) ml_terms_sort(Lo, A - 1);
	if (B + 1 < Hi) ml_terms_sort(B + 1, Hi);
}

static ml_polynomial_t *ml_polynomial_mul(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	int Count = A->Count * B->Count;
	ml_polynomial_t *C = xnew(ml_polynomial_t, Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	for (int IA = 0; IA < CA; ++IA) for (int IB = 0; IB < CB; ++IB) {
		TC->Coeff = TA[IA].Coeff * TB[IB].Coeff;
		TC->Factors = ml_factors_mul(TA[IA].Factors, TB[IB].Factors);
		++TC;
	}
	ml_terms_sort(C->Terms, TC - 1);
	TC = C->Terms;
	ml_term_t *TC2 = TC;
	for (int I = 0; I < Count; ++I, ++TC2) if (TC2->Coeff) *TC++ = *TC2;
	C->Count = TC - C->Terms;
	return C;
}

static int ml_term_div(ml_term_t *A, ml_term_t *B, ml_term_t *C) {
	const ml_factor_t *FA = A->Factors->Factors, *FB = B->Factors->Factors;
	int CA = A->Factors->Count, CB = B->Factors->Count;
	ml_factors_t *Factors = xnew(ml_factors_t, CA, ml_factor_t);
	ml_factor_t *FC = Factors->Factors;
	int Degree = 0;
	while (CA && CB) {
		if (FA->Variable > FB->Variable) return 0;
		if (FA->Variable < FB->Variable) {
			Degree += FA->Degree;
			*FC++ = *FA++; --CA;
		} else {
			int D = FA->Degree - FB->Degree;
			if (D < 0) return 0;
			if (D > 0) {
				Degree += D;
				FC->Degree = D;
				FC->Variable = FA->Variable;
				++FC;
			}
			++FA; ++FB;
			--CA; --CB;
		}
	}
	if (CB) return 0;
	while (CA) {
		Degree += FA->Degree;
		*FC++ = *FA++;
		--CA;
	}
	if (Degree) {
		Factors->Degree = Degree;
		Factors->Count = FC - Factors->Factors;
		C->Factors = Factors;
	} else {
		C->Factors = Constant;
	}
	C->Coeff = A->Coeff / B->Coeff;
	return 1;
}

typedef struct {
	ml_polynomial_t *Q, *R;
} ml_quotient_t;

static ml_quotient_t ml_polynomial_quotient(ml_polynomial_t *A, ml_polynomial_t *B) {
	// TODO: optimize this code to prevent unneccessary allocations
	ml_polynomial_t *D = xnew(ml_polynomial_t, 1, ml_term_t);
	D->Type = MLPolynomialT;
	D->Count = 1;
	ml_polynomial_t *Q = NULL;
	for (int I = 0; I < A->Count;) {
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "A = (");
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, "), B = (");
		ml_polynomial_write(Buffer, B);
		ml_stringbuffer_printf(Buffer, "), Q = (");
		if (Q) ml_polynomial_write(Buffer, Q);
		ml_stringbuffer_printf(Buffer, ")");
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		if (ml_term_div(A->Terms + I, B->Terms, D->Terms)) {
			if (Q) {
				Q = ml_polynomial_add(Q, D);
			} else {
				Q = xnew(ml_polynomial_t, 1, ml_term_t);
				Q->Type = MLPolynomialT;
				Q->Count = 1;
				Q->Terms[0] = D->Terms[0];
			}
			A = ml_polynomial_sub(A, ml_polynomial_mul(B, D));
		} else {
			++I;
		}
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "A = (");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, "), B = (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, "), Q = (");
	if (Q) ml_polynomial_write(Buffer, Q);
	ml_stringbuffer_printf(Buffer, ")");
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_quotient_t){Q, A->Count ? A : NULL};
}

static ml_polynomial_t *ml_polynomial_spol(ml_polynomial_t *A, ml_polynomial_t *B) {
	int CA = A->Terms[0].Factors->Count;
	int CB = B->Terms[0].Factors->Count;
	const ml_factor_t *FA = A->Terms[0].Factors->Factors;
	const ml_factor_t *FB = B->Terms[0].Factors->Factors;
	ml_factors_t *MA = xnew(ml_factors_t, CB, ml_factor_t);
	ml_factors_t *MB = xnew(ml_factors_t, CA, ml_factor_t);
	ml_factor_t *MFA = MA->Factors, *MFB = MB->Factors;
	while (CA && CB) {
		if (FA->Variable < FB->Variable) {
			MB->Degree += FA->Degree;
			*MFB++ = *FA++;
			--CA;
		} else if (FA->Variable > FB->Variable) {
			MA->Degree += FB->Degree;
			*MFA++ = *FB++;
			--CB;
		} else if (FA->Degree > FB->Degree) {
			MFB->Variable = FA->Variable;
			MFB->Degree = FA->Degree - FB->Degree;
			MB->Degree += MFB->Degree;
			++MFB;
			++FA; ++FB;
			--CA; --CB;
		} else if (FA->Degree < FB->Degree) {
			MFA->Variable = FB->Variable;
			MFA->Degree = FB->Degree - FA->Degree;
			MA->Degree += MFA->Degree;
			++MFA;
			++FA; ++FB;
			--CA; --CB;
		} else {
			++FA; ++FB;
			--CA; --CB;
		}
	}
	while (CA) {
		MB->Degree += FA->Degree;
		*MFB++ = *FA++;
		--CA;
	}
	while (CB) {
		MA->Degree += FB->Degree;
		*MFA++ = *FB++;
		--CB;
	}
	MA->Count = MFA - MA->Factors;
	MB->Count = MFB - MB->Factors;
	ml_coeff_t X = B->Terms[0].Coeff / A->Terms[0].Coeff;
	ml_polynomial_t *XA = xnew(ml_polynomial_t, A->Count, ml_term_t);
	XA->Type = MLPolynomialT;
	XA->Count = A->Count;
	for (int I = 0; I < A->Count; ++I) {
		XA->Terms[I].Coeff = A->Terms[I].Coeff * X;
		XA->Terms[I].Factors = ml_factors_mul(A->Terms[I].Factors, MA);
	}
	ml_terms_sort(XA->Terms, XA->Terms + XA->Count - 1);
	ml_polynomial_t *XB = xnew(ml_polynomial_t, B->Count, ml_term_t);
	XB->Type = MLPolynomialT;
	XB->Count = B->Count;
	for (int I = 0; I < B->Count; ++I) {
		XB->Terms[I].Coeff = B->Terms[I].Coeff;
		XB->Terms[I].Factors = ml_factors_mul(B->Terms[I].Factors, MB);
	}
	ml_terms_sort(XB->Terms, XB->Terms + XB->Count - 1);
	return ml_polynomial_sub(XA, XB);
}

static ml_polynomial_t *ml_polynomial_reduce(ml_polynomial_t *A, ml_polynomial_t *B) {
	for (int I = 0; I < A->Count; ++I) {
		int CA = A->Terms[I].Factors->Count;
		int CB = B->Terms[0].Factors->Count;
		const ml_factor_t *FA = A->Terms[I].Factors->Factors;
		const ml_factor_t *FB = B->Terms[0].Factors->Factors;
		ml_factors_t *MB = xnew(ml_factors_t, CA, ml_factor_t);
		ml_factor_t *MFB = MB->Factors;
		while (CA && CB) {
			if (FA->Variable < FB->Variable) {
				MB->Degree += FA->Degree;
				*MFB++ = *FA++;
				--CA;
			} else if (FA->Variable > FB->Variable) {
				goto next;
			} else if (FA->Degree > FB->Degree) {
				MFB->Variable = FA->Variable;
				MFB->Degree = FA->Degree - FB->Degree;
				MB->Degree += MFB->Degree;
				++MFB;
				++FA; ++FB;
				--CA; --CB;
			} else if (FA->Degree < FB->Degree) {
				goto next;
			} else {
				++FA; ++FB;
				--CA; --CB;
			}
		}
		if (CB) goto next;
		while (CA) {
			MB->Degree += FA->Degree;
			*MFB++ = *FA++;
			--CA;
		}
		MB->Count = MFB - MB->Factors;
		ml_polynomial_t *XB = xnew(ml_polynomial_t, B->Count, ml_term_t);
		XB->Type = MLPolynomialT;
		XB->Count = B->Count;
		ml_coeff_t X = A->Terms[I].Coeff / B->Terms[0].Coeff;
		for (int I = 0; I < B->Count; ++I) {
			XB->Terms[I].Coeff = X * B->Terms[I].Coeff;
			XB->Terms[I].Factors = ml_factors_mul(B->Terms[I].Factors, MB);
		}
		ml_terms_sort(XB->Terms, XB->Terms + XB->Count - 1);
		return ml_polynomial_sub(A, XB);
	next:;
	}
	return NULL;
}

ML_METHOD(MLPolynomialT, MLStringT) {
//<Var
//>polynomial
// Returns the polynomial corresponding to the variable :mini:`Var`.
//$= let X := polynomial("x"), Y := polynomial("y")
//$= let P := (X - Y) ^ 4
//$= P(y is 3)
	const char *Name = ml_string_value(Args[0]);
	int *Slot = (int *)stringmap_slot(Variables, Name);
	if (!Slot[0]) {
		int Index = Slot[0] = Variables->Size;
		Names = GC_realloc(Names, Index * sizeof(const char *));
		Names[Index - 1] = Name;
	}
	ml_factors_t *Factors = xnew(ml_factors_t, 1, ml_factor_t);
	Factors->Degree = 1;
	Factors->Count = 1;
	Factors->Factors->Variable = Slot[0];
	Factors->Factors->Degree = 1;
	ml_polynomial_t *Poly = xnew(ml_polynomial_t, 1, ml_term_t);
	Poly->Type = MLPolynomialT;
	Poly->Count = 1;
	Poly->Terms->Factors = Factors;
	Poly->Terms->Coeff = 1;
	return (ml_value_t *)Poly;
}

ML_METHOD("degree", MLPolynomialT, MLStringT) {
//<Poly
//<Var
//>integer
// Returns the highest degree of :mini:`Var` in :mini:`Poly`.
//$- let X := polynomial("x")
//$= (X ^ 2 + (3 * X) + 2):degree("x")
	const ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int Variable = (intptr_t)stringmap_search(Variables, ml_string_value(Args[1]));
	if (!Variable) return ml_integer(0);
	int Degree = 0;
	const ml_term_t *TA = A->Terms;
	for (int I = A->Count; --I >= 0; ++TA) {
		const ml_factor_t *FA = TA->Factors->Factors;
		for (int J = TA->Factors->Count; --J >= 0; ++FA) {
			if (FA->Variable == Variable) {
				if (Degree < FA->Degree) Degree = FA->Degree;
			}
		}
	}
	return ml_integer(Degree);
}

ML_METHOD("coeff", MLPolynomialT, MLStringT, MLIntegerT) {
//<Poly
//<Var
//<Degree
//>number|polynomial
// Returns the coefficient of :mini:`Var ^ Degree` in :mini:`Poly`.
//$- let X := polynomial("x")
//$= (X ^ 2 + (3 * X) + 2):coeff("x", 1)
	const ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int Variable = (intptr_t)stringmap_search(Variables, ml_string_value(Args[1]));
	if (!Variable) return ml_real(0);
	int Degree = ml_integer_value(Args[2]);
	const ml_term_t *TA = A->Terms;
	ml_polynomial_t *B = NULL;
	ml_term_t *TB = NULL;
	ml_coeff_t Coeff = 0;
	if (Degree) {
		for (int I = A->Count; --I >= 0; ++TA) {
			const ml_factor_t *FA = TA->Factors->Factors;
			for (int J = TA->Factors->Count; --J >= 0; ++FA) {
				if (FA->Variable == Variable && FA->Degree == Degree) {
					if (TA->Factors->Count == 1) {
						if (TB) {
							TB->Coeff = TA->Coeff;
							TB->Factors = Constant;
							++TB;
						} else {
							Coeff = TA->Coeff;
						}
					} else {
						if (!TB) {
							if (abs(Coeff) > 0) {
								B = xnew(ml_polynomial_t, I + 2, ml_term_t);
								TB = B->Terms;
								TB->Coeff = Coeff;
								TB->Factors = Constant;
								++TB;
							} else {
								B = xnew(ml_polynomial_t, I + 1, ml_term_t);
								TB = B->Terms;
							}
							B->Type = MLPolynomialT;
						}
						ml_factors_t *Factors = xnew(ml_factors_t, TA->Factors->Count - 1, ml_factor_t);
						ml_factor_t *FB = Factors->Factors;
						for (const ml_factor_t *FA2 = TA->Factors->Factors; FA2 != FA; ++FA2) *FB++ = *FA2;
						const ml_factor_t *EA = TA->Factors->Factors + TA->Factors->Count;
						for (const ml_factor_t *FA2 = FA + 1; FA2 != EA; ++FA2) *FB++ = *FA2;
						Factors->Count = TA->Factors->Count - 1;
						Factors->Degree = TA->Factors->Degree - Degree;
						TB->Coeff = TA->Coeff;
						TB->Factors = Factors;
						++TB;
					}
				}
			}
		}
	} else {
		for (int I = A->Count; --I >= 0; ++TA) {
			const ml_factor_t *FA = TA->Factors->Factors;
			for (int J = TA->Factors->Count; --J >= 0; ++FA) {
				if (FA->Variable == Variable) goto next;
			}
			if (TB) {
				*TB++ = *TA;
			} else if (TA->Factors == Constant) {
				Coeff = TA->Coeff;
			} else {
				if (abs(Coeff) > 0) {
					B = xnew(ml_polynomial_t, I + 2, ml_term_t);
					TB = B->Terms;
					TB->Coeff = Coeff;
					TB->Factors = Constant;
					++TB;
				} else {
					B = xnew(ml_polynomial_t, I + 1, ml_term_t);
					TB = B->Terms;
				}
				B->Type = MLPolynomialT;
				*TB++ = *TA;
			}
			next:;
		}
	}
	if (B) {
		B->Count = TB - B->Terms;
		return (ml_value_t *)B;
	}
	return ml_coeff(Coeff);
}

ML_METHOD("d", MLPolynomialT, MLStringT) {
//<Poly
//<Var
//>number|polynomial
// Returns the derivative of :mini:`Poly` w.r.t. :mini:`Var`.
//$- let X := polynomial("x")
//$= (X ^ 2 + (3 * X) + 2):d("x")
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int Variable = (intptr_t)stringmap_search(Variables, ml_string_value(Args[1]));
	if (!Variable) return ml_real(0);
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms;
	ml_term_t *TC = C->Terms;
	for (int I = A->Count; --I >= 0; ++TA) {
		int N = TA->Factors->Count;
		for (int J = 0; J < N; ++J) {
			const ml_factor_t *FA = TA->Factors->Factors + J;
			if (FA->Variable == Variable) {
				TC->Coeff = TA->Coeff * FA->Degree;
				if (FA->Degree == 1) {
					ml_factors_t *F = xnew(ml_factors_t, N - 1, ml_factor_t);
					for (int K = 0; K < J; ++K) F->Factors[K] = TA->Factors->Factors[K];
					for (int K = J + 1; K < N; ++K) F->Factors[K  - 1] = TA->Factors->Factors[K];
					F->Count = N - 1;
					F->Degree = TA->Factors->Degree - 1;
					TC->Factors = F;
				} else {
					ml_factors_t *F = xnew(ml_factors_t, N, ml_factor_t);
					for (int K = 0; K < N; ++K) F->Factors[K] = TA->Factors->Factors[K];
					--F->Factors[J].Degree;
					F->Count = N;
					F->Degree = TA->Factors->Degree - 1;
					TC->Factors = F;
				}
				++TC;
			}
		}
	}
	C->Count = TC - C->Terms;
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("+", MLPolynomialT, MLNumberT) {
//<A
//<B
//>polynomial
// Returns :mini:`A + B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_coeff_t N = ml_coeff_value(Args[1]);
	if (abs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		ml_coeff_t Sum = A->Terms[CA - 1].Coeff + N;
		if (abs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_write(Buffer, "(", 1);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("+", MLNumberT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A + B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	ml_coeff_t N = ml_coeff_value(Args[0]);
	if (abs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		ml_coeff_t Sum = A->Terms[CA - 1].Coeff + N;
		if (abs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) + (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) + (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) + (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("+", MLPolynomialT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A + B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_add(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") + (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("-", MLPolynomialT, MLNumberT) {
//<A
//<B
//>polynomial
// Returns :mini:`A - B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_coeff_t N = ml_coeff_value(Args[1]);
	if (abs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		ml_coeff_t Sum = A->Terms[CA - 1].Coeff - N;
		if (abs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = -N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_write(Buffer, "(", 1);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("-", MLNumberT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A - B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	ml_coeff_t N = ml_coeff_value(Args[0]);
	int CA = A->Count;
	if (abs(N) < DBL_EPSILON) {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA;
		for (int I = 0; I < CA; ++I) {
			B->Terms[I].Factors = A->Terms[I].Factors;
			B->Terms[I].Coeff = -A->Terms[I].Coeff;
		}
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) - (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	} else if (A->Terms[CA - 1].Factors->Count == 0) {
		ml_coeff_t Sum = N - A->Terms[CA - 1].Coeff;
		if (abs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			for (int I = 0; I < CA; ++I) {
				B->Terms[I].Factors = A->Terms[I].Factors;
				B->Terms[I].Coeff = -A->Terms[I].Coeff;
			}
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) - (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			for (int I = 0; I < CA - 1; ++I) {
				B->Terms[I].Factors = A->Terms[I].Factors;
				B->Terms[I].Coeff = -A->Terms[I].Coeff;
			}
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) - (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		for (int I = 0; I < CA; ++I) {
			B->Terms[I].Factors = A->Terms[I].Factors;
			B->Terms[I].Coeff = -A->Terms[I].Coeff;
		}
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) - (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("-", MLPolynomialT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A - B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_sub(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") - (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("*", MLPolynomialT, MLNumberT) {
//<A
//<B
//>polynomial
// Returns :mini:`A * B`.
	ml_coeff_t N = ml_coeff_value(Args[1]);
	if (abs(N) < DBL_EPSILON) return Args[1];
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = A->Terms[I].Coeff * N;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, "(", 1);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") * (%g) = ", N);
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_METHOD("*", MLNumberT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A * B`.
	ml_coeff_t N = ml_coeff_value(Args[0]);
	if (abs(N) < DBL_EPSILON) return Args[0];
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = N * A->Terms[I].Coeff;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(%g) * (", N);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_METHOD("*", MLPolynomialT, MLPolynomialT) {
//<A
//<B
//>polynomial
// Returns :mini:`A * B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_mul(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") * (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)C;
}

ML_METHOD("^", MLPolynomialT, MLIntegerT) {
//<A
//<B
//>polynomial
// Returns :mini:`A ^ B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int N = ml_integer_value(Args[1]);
	if (N < 0) return ml_error("RangeError", "Negative powers not implemented yet");
	if (N == 0) return ml_real(1);
	if (A->Count == 1) {
		int C = A->Terms->Factors->Count;
		const ml_factors_t *FA = A->Terms->Factors;
		ml_factors_t *FB = xnew(ml_factors_t, C, ml_factor_t);
		FB->Count = C;
		FB->Degree = FA->Degree * N;
		memcpy(FB->Factors, FA->Factors, C * sizeof(ml_factor_t));
		for (int I = 0; I < C; ++I) FB->Factors[I].Degree *= N;
		ml_polynomial_t *B = xnew(ml_polynomial_t, 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = 1;
		B->Terms->Coeff = pow(A->Terms->Coeff, N);
		B->Terms->Factors = FB;
		return (ml_value_t *)B;
	}
	ml_polynomial_t *S = A, *B = NULL;
	while (N) {
		if (N % 2) B = B ? ml_polynomial_mul(B, S) : S;
		N /= 2;
		S = ml_polynomial_mul(S, S);
	}
	return (ml_value_t *)B;
}

ML_METHOD("append", MLStringBufferT, MLPolynomialT) {
//<Buffer
//<Poly
// Appends a representation of :mini:`Poly` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_polynomial_t *Poly = (ml_polynomial_t *)Args[1];
	ml_polynomial_write(Buffer, Poly);
	return MLSome;
}

ML_METHOD("/", MLPolynomialT, MLNumberT) {
//<A
//<B
//>polynomial
// Returns :mini:`A / B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_coeff_t N = ml_coeff_value(Args[1]);
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = A->Terms[I].Coeff / N;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, "(", 1);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") / (%g) = ", N);
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_TYPE(MLPolynomialRationalT, (), "polynomial::rational");

static ml_value_t *ml_polynomial_div(ml_polynomial_t *A, ml_polynomial_t *B) {
	ml_polynomial_t *T = A, *G = B;
	int Cycle = 0;
	for (;;) {
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "T = (");
		ml_polynomial_write(Buffer, T);
		ml_stringbuffer_printf(Buffer, "), G = (");
		ml_polynomial_write(Buffer, G);
		ml_stringbuffer_printf(Buffer, ")");
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		ml_quotient_t D = ml_polynomial_quotient(T, G);
		if (!D.R) {
			if (G == B) {
				if (D.Q->Count == 1 && D.Q->Terms->Factors->Count == 0) {
					return ml_real(D.Q->Terms->Coeff);
				} else {
					return (ml_value_t *)D.Q;
				}
			} else {
				break;
			}
		} else if (!D.Q) {
			if (++Cycle == 2) {
				G = ml_polynomial_const(1);
				break;
			}
		} else {
			Cycle = 0;
		}
		T = G; G = D.R;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "A = (");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, "), B = (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, "), G = (");
	ml_polynomial_write(Buffer, G);
	ml_stringbuffer_printf(Buffer, ")");
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	ml_polynomial_rational_t *C = new(ml_polynomial_rational_t);
	C->Type = MLPolynomialRationalT;
	if (G->Count == 1 && G->Terms->Factors->Count == 0) {
		C->A = A;
		C->B = B;
	} else {
		C->A = ml_polynomial_quotient(A, G).Q;
		C->B = ml_polynomial_quotient(B, G).Q;
	}
	return (ml_value_t *)C;
}

ML_METHOD("/", MLNumberT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_t *A = ml_polynomial_const(ml_coeff_value(Args[0]));
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A, B);
}

ML_METHOD("/", MLPolynomialT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A, B);
}

ML_METHOD("spol", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return (ml_value_t *)ml_polynomial_spol(A, B);
}

ML_METHOD("red", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return (ml_value_t *)ml_polynomial_reduce(A, B) ?: MLNil;
}

ML_METHOD("+", MLNumberT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A + B`.
	ml_polynomial_t *A = ml_polynomial_const(ml_coeff_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLNumberT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A + B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_coeff_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_add(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("+", MLPolynomialT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A + B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A + B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A + B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_add(ml_polynomial_mul(A->A, B->B), ml_polynomial_mul(A->B, B->A)),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("-", MLNumberT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A - B`.
	ml_polynomial_t *A = ml_polynomial_const(ml_coeff_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLNumberT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A - B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_coeff_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_sub(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("-", MLPolynomialT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A - B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A - B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A - B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_sub(ml_polynomial_mul(A->A, B->B), ml_polynomial_mul(A->B, B->A)),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("*", MLNumberT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A * B`.
	ml_polynomial_t *A = ml_polynomial_const(ml_coeff_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->A), B->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLNumberT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A * B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_coeff_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_mul(A->A, B), A->B);
}

ML_METHOD("*", MLPolynomialT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A * B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->A), B->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A * B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A->A, B), A->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A * B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_mul(A->A, B->A),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("/", MLNumberT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_t *A = ml_polynomial_const(ml_coeff_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->B), B->A);
}

ML_METHOD("/", MLPolynomialRationalT, MLNumberT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_coeff_value(Args[1]));
	return ml_polynomial_div(A->A, ml_polynomial_mul(A->B, B));
}

ML_METHOD("/", MLPolynomialT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->B), B->A);
}

ML_METHOD("/", MLPolynomialRationalT, MLPolynomialT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A->A, ml_polynomial_mul(A->B, B));
}

ML_METHOD("/", MLPolynomialRationalT, MLPolynomialRationalT) {
//<A
//<B
//>polynomial::rational
// Returns :mini:`A / B`.
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_mul(A->A, B->B),
		ml_polynomial_mul(A->B, B->A)
	);
}

ML_METHOD("append", MLStringBufferT, MLPolynomialRationalT) {
//<Buffer
//<Poly
// Appends a representation of :mini:`Poly` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_polynomial_rational_t *Rat = (ml_polynomial_rational_t *)Args[1];
	ml_stringbuffer_write(Buffer, "[", 1);
	ml_polynomial_write(Buffer, Rat->A);
	ml_stringbuffer_write(Buffer, " / ", 3);
	ml_polynomial_write(Buffer, Rat->B);
	ml_stringbuffer_write(Buffer, "]", 1);
	return MLSome;
}

#ifdef ML_COMPLEX

// Code adapted from https://github.com/yairchu/quartic

#define MAX_DEGREE 4

static double stableness_score(complex double a, complex double b);
static int solve_normalized_poly(int degree, const complex double *poly, complex double *results);
static void calc_shifted_coefs(complex double shift, int degree, const complex double *src, complex double *dst);
static void calc_binomials(int num_binoms, int stride, double *dst);
static void calc_powers(complex double x, int max_power, complex double *dst);
static int solve_depressed_poly(int degree, const complex double *poly, complex double *results);
static int solve_depressed_quartic(const complex double *poly, complex double *results);
static int solve_depressed_cubic(const complex double *poly, complex double *results);
static int solve_depressed_quadratic(const complex double *poly, complex double *results);

/* poly: pointer to coefficients array of size degree + 1.
 * results: pointer to results output array of size degree.
 */
int solve_poly(int degree, const complex double *poly, complex double *results) {
	complex double normalized_poly[MAX_DEGREE + 1];
	int i;
	const complex double a = poly[degree];
	if (degree == 0)
		return 0;
	if (cabs(a) < DBL_EPSILON) return solve_poly(degree - 1, poly, results);
	if (degree > MAX_DEGREE) return -1;
	if (degree > 2 && stableness_score(poly[degree], poly[degree - 1]) > stableness_score(poly[0], poly[1])) {
		complex double rev_poly[MAX_DEGREE + 1];
		int num_results;
		for (i = 0; i <= degree; ++i)
			rev_poly[i] = poly[degree - i];
		num_results = solve_poly(degree, rev_poly, results);
		for (i = 0; i < num_results; ++i)
			results[i] = 1 / results[i];
		return num_results;
	}
	for (i = 0; i < degree; ++i)
		normalized_poly[i] = poly[i] / a;
	normalized_poly[degree] = 1.0;
	return solve_normalized_poly(degree, normalized_poly, results);
}

static double stableness_score(complex double a, complex double b) {
	const double t = sqrt(cabs(a) / cabs(b));
	return t + 1.0 / t;
}

/* Normalized polynomials have the form of
 *   x^n + a*x^(n-1) + ..
 * The coefficient for x^n is one.
 * solve_normalized_poly does expect to get this coefficient despite it being known.
 */
static int solve_normalized_poly(int degree, const complex double *poly, complex double *results) {
	const complex double shift = -poly[degree - 1] / degree;
	complex double shifted_coefs[MAX_DEGREE + 1];
	int i, num_results;
	calc_shifted_coefs(shift, degree, poly, shifted_coefs);
	num_results = solve_depressed_poly(degree, shifted_coefs, results);
	for (i = 0; i < num_results; ++i) results[i] += shift;
	return num_results;
}

static void calc_shifted_coefs(complex double shift, int degree, const complex double *src, complex double *dst) {
	double binomials[MAX_DEGREE + 1][MAX_DEGREE + 1];
	complex double shift_powers[MAX_DEGREE + 1];
	int dst_i, src_i;
	for (dst_i = 0; dst_i <= degree; ++dst_i)
		dst[dst_i] = 0.0;
	calc_binomials(degree+1, sizeof(binomials[0]) / sizeof(binomials[0][0]), binomials[0]);
	calc_powers(shift, degree, shift_powers);
	for (src_i = 0; src_i <= degree; ++src_i) {
		for (dst_i = 0; dst_i <= src_i; ++dst_i) {
			dst[dst_i] = dst[dst_i] + binomials[src_i][dst_i] * src[src_i] + shift_powers[src_i - dst_i];
		}
	}
}

static void calc_binomials(int num_binoms, int stride, double *dst) {
	int row;
	for (row = 0; row < num_binoms; ++row) {
		const int row_idx = row * stride;
		const int prev_row_idx = (row - 1) * stride;
		int col;
		dst[row_idx] = 1;
		for (col = 1; col < row; ++col) {
			dst[row_idx + col] = dst[prev_row_idx + col - 1] + dst[prev_row_idx + col];
		}
		dst[row_idx + row] = 1;
	}
}

static void calc_powers(complex double x, int max_power, complex double *dst) {
	int i;
	dst[0] = 1.0;
	if (max_power >= 1) dst[1] = x;
	for (i = 2; i <= max_power; ++i) dst[i] = x * dst[i - 1];
}

/* Depressed polynomials have the form of:
 *   x^n + a*x^(n-2) + ..
 * The coefficient for x^n is 1 and for x^(n-1) is zero.
 * So it gets 3 coefficients for a depressed quartic polynom.
 */
static int solve_depressed_poly(int degree, const complex double *poly, complex double *results) {
	if (degree > 0 && cabs(poly[0]) < DBL_EPSILON) {
		results[0] = 0.0;
		return 1 + solve_depressed_poly(degree - 1, poly + 1, results + 1);
	}
	switch (degree) {
	case 4:
		return solve_depressed_quartic(poly, results);
	case 3:
		return solve_depressed_cubic(poly, results);
	case 2:
		return solve_depressed_quadratic(poly, results);
	case 1:
		results[0] = 0.0;
		return 1;
	case 0:
		return 0;
	default:
		return -1;
	}
}

/* Based on http://en.wikipedia.org/wiki/Quartic_function#Quick_and_memorable_solution_from_first_principles */
static int solve_depressed_quartic(const complex double *poly, complex double *results)
{
	complex double helper_cubic[4];
	complex double helper_results[3];
	complex double quadratic_factor[3];
	complex double p, c_plus_p_sqr, d_div_p;
	const complex double e = poly[0];
	const complex double d = poly[1];
	const complex double c = poly[2];
	double helper_norm, t;
	int num_helper_results, num_results, best_helper_result, i;

	if (cabs(d) < DBL_EPSILON) {
		int i, num_quad_results;
		complex double quadratic[3];
		complex double quadratic_results[2];
		quadratic[0] = e;
		quadratic[1] = c;
		quadratic[2] = 1.0;
		num_quad_results = solve_poly(2, quadratic, quadratic_results);
		for (i = 0; i < num_quad_results; ++i) {
			const complex double s = csqrt(quadratic_results[i]);
			results[2*i] = -s;
			results[2*i + 1] = s;
		}
		return 2 * num_quad_results;
	}

	helper_cubic[0] = -d * d;
	helper_cubic[1] = c * c - 4 * e;
	helper_cubic[2] = 2 * c;
	helper_cubic[3] = 1;
	num_helper_results = solve_poly(3, helper_cubic, helper_results);
	if (num_helper_results < 1) return 0;

	// Pick the result of helper_cubic which has the highest norm,
	// For more stable calculation. Fixes https://github.com/yairchu/quartic/issues/2
	best_helper_result = 0;
	helper_norm = cabs(helper_results[0]);
	for (i = 1; i < num_helper_results; ++i) {
		t = cabs(helper_results[i]);
		if (t > helper_norm) {
			helper_norm = t;
			best_helper_result = i;
		}
	}

	p = csqrt(helper_results[best_helper_result]);
	c_plus_p_sqr = c + p * p;
	d_div_p = d / p;
	quadratic_factor[0] = c_plus_p_sqr - d_div_p;
	quadratic_factor[1] = 2 * p;
	quadratic_factor[2] = 2;
	num_results = solve_poly(2, quadratic_factor, results);
	quadratic_factor[0] = c_plus_p_sqr + d_div_p;
	quadratic_factor[1] = -quadratic_factor[1];
	return num_results + solve_poly(2, quadratic_factor, results + num_results);
}

/* Based on http://en.wikipedia.org/wiki/Cubic_equation#Cardano.27s_method */
static int solve_depressed_cubic(const complex double *poly, complex double *results) {
	const complex double q = poly[0];
	const complex double p = poly[1];
	if (cabs(p) < DBL_EPSILON) {
		results[0] = cpow(-q, 1.0 / 3.0);
		results[1] = results[0];
		results[2] = results[1];
		return 3;
	}
	complex double t = q * q / 4 + p * p * p / 27;
	complex double z = -0.5 + 0.5 * sqrt(3.0) * _Complex_I;
	results[0] = cpow(-q / 2 + csqrt(t), 1.0 / 3.0);
	results[1] = results[0] * z;
	results[2] = results[1] * z;
	return 3;
}

static int solve_depressed_quadratic(const complex double *poly, complex double *results) {
	const complex double t = csqrt(-poly[0]);
	results[0] = -t;
	results[1] = t;
	return 2;
}

static void ml_roots_quadratic(complex double Coeffs[], complex double Roots[]) {
	complex double A = Coeffs[1] / Coeffs[0];
	complex double B = Coeffs[2] / Coeffs[0];
	complex double A0 = -A / 2;
	complex double D = (A0 * A0) - B;
	complex double SD = csqrt(D);
	Roots[0] = A0 - SD;
	Roots[1] = A0 + SD;
}

static inline complex double ccbrt(complex double X) {
	if (creal(X) >= 0) return cpow(X, 1 / 3);
	return -cpow(-X, 1 / 3);
}

static void ml_roots_cubic(complex double Coeffs[], complex double Roots[]) {
	complex double A = Coeffs[1] / Coeffs[0];
	complex double B = Coeffs[2] / Coeffs[0];
	complex double C = Coeffs[3] / Coeffs[0];
	complex double A13 = A / 3;
	complex double A2 = A13 * A13;
	const double Sqrt3 = sqrt(3);
	complex double F = B / 3 - A2;
	complex double G = A13 * (2 * A2 - B) + C;
	complex double H = G * G / 4 + F * F * F;
	if (cabs(F) < DBL_EPSILON && cabs(G) < DBL_EPSILON && cabs(H) < DBL_EPSILON) {
		if (fabs(cimag(C)) < DBL_EPSILON) {
			Roots[0] = Roots[1] = Roots[2] = -cbrt(creal(C));
		} else {
			Roots[0] = Roots[1] = Roots[2] = -ccbrt(C);
		}
	} else {
		complex double SqrtH = csqrt(H);
		complex double S = ccbrt(-G / 2 + SqrtH);
		complex double U = ccbrt(-G / 2 - SqrtH);
		double SaddU = S + U;
		double SsubU = S - U;
		Roots[0] = SaddU - A13;
		Roots[1] = -SaddU / 2 - A13 + SsubU * Sqrt3 * _Complex_I / 2;
		Roots[2] = -SaddU / 2 - A13 - SsubU * Sqrt3 * _Complex_I / 2;
	}
}

static void ml_roots_quartic(complex double Coeffs[], complex double Roots[]) {
	complex double A = Coeffs[1] / Coeffs[0];
	complex double B = Coeffs[2] / Coeffs[0];
	complex double C = Coeffs[3] / Coeffs[0];
	complex double D = Coeffs[4] / Coeffs[0];
	complex double A0 = A / 4;
	complex double A02 = A0 * A0;
	complex double P = 3 * A02 - B / 2;
	complex double Q = A * A02 - B * A0 + C / 2;
	complex double R = 3 * A02 * A02 - B * A02 + C * A0 - D;
	complex double Cubic[4] = {1, P, R, P * R - Q * Q / 2};
	ml_roots_cubic(Cubic, Roots);
	complex double Z0 = Roots[0];
	complex double S = csqrt(2 * P + 2 * creal(Z0));
	complex double T = cabs(S) < DBL_EPSILON ? Z0 * Z0 + R : -Q / S;
	complex double Quadratic1[3] = {1, S, Z0 + T};
	ml_roots_quadratic(Quadratic1, Roots);
	complex double Quadratic2[3] = {1, -S, Z0 - T};
	ml_roots_quadratic(Quadratic2, Roots + 2);
	Roots[0] -= A0;
	Roots[1] -= A0;
	Roots[2] -= A0;
	Roots[3] -= A0;
}

ML_FUNCTION(MLPolynomialRoots) {
	complex double Coeffs[5];
	complex double Roots[4];
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(0, MLComplexT);
	Coeffs[0] = ml_complex_value(Args[0]);
	ML_CHECK_ARG_TYPE(1, MLComplexT);
	Coeffs[1] = ml_complex_value(Args[1]);
	ML_CHECK_ARG_TYPE(2, MLComplexT);
	Coeffs[2] = ml_complex_value(Args[2]);
	if (Count > 3) {
		ML_CHECK_ARG_TYPE(3, MLComplexT);
		Coeffs[3] = ml_complex_value(Args[3]);
		if (Count > 4) {
			ML_CHECK_ARG_TYPE(4, MLComplexT);
			Coeffs[4] = ml_complex_value(Args[4]);
			if (Count > 5) return ml_error("CallError", "Too many arguments");
			ml_roots_quartic(Coeffs, Roots);
		} else {
			ml_roots_cubic(Coeffs, Roots);
		}
	} else {
		ml_roots_quadratic(Coeffs, Roots);
	}
	int NumRoots = solve_poly(Count, Coeffs, Roots);
	ml_value_t *Result = ml_list();
	for (int I = 0; I < NumRoots; ++I) ml_list_put(Result, ml_complex(Roots[I]));
	return Result;
}

#endif

void ml_polynomial_init(stringmap_t *Globals) {
#include "ml_polynomial_init.c"
#ifdef ML_COMPLEX
	stringmap_insert(MLPolynomialT->Exports, "roots", MLPolynomialRoots);
#endif
	if (Globals) {
		stringmap_insert(Globals, "polynomial", MLPolynomialT);
	}
}
