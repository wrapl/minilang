#include "../minilang.h"
#include "../ml_library.h"
#include "kiwi/kiwi.h"
#include <gc/gc_cpp.h>

ML_TYPE(KiwiVariableT, (), "kiwi-variable");

struct kiwi_variable_t : public gc {
	const ml_type_t *Type = KiwiVariableT;
	kiwi::Variable Value;

	kiwi_variable_t(const char *Name) : Value(Name) {}
};

ML_FUNCTION(KiwiVariable) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return (ml_value_t *)new kiwi_variable_t(ml_string_value(Args[0]));
}

ML_METHOD("value", KiwiVariableT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	return ml_real(V->Value.value());
}

ML_METHOD(MLStringOfMethod, KiwiVariableT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "?%s", V->Value.name().c_str());
	return ml_stringbuffer_value(Buffer);
}

ML_TYPE(KiwiExpressionT, (), "kiwi-expression");

struct kiwi_expression_t : public gc {
	const ml_type_t *Type = KiwiExpressionT;
	kiwi::Expression Value;

	kiwi_expression_t(kiwi::Expression E) : Value(E) {}
};

ML_METHOD(MLStringOfMethod, KiwiExpressionT) {
	kiwi_expression_t *E = (kiwi_expression_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "%g", E->Value.constant());
	for (auto Term : E->Value.terms()) {
		if (Term.coefficient() < 0.0) {
			ml_stringbuffer_addf(Buffer, " - %g", -Term.coefficient());
		} else {
			ml_stringbuffer_addf(Buffer, " + %g", Term.coefficient());
		}
		ml_stringbuffer_addf(Buffer, "%s", Term.variable().name().c_str());
	}
	return ml_stringbuffer_value(Buffer);
}

ML_TYPE(KiwiConstraintT, (), "kiwi-constraint");

struct kiwi_constraint_t : public gc {
	const ml_type_t *Type = KiwiConstraintT;
	kiwi::Constraint Value;

	kiwi_constraint_t(kiwi::Constraint C) : Value(C) {}
};

ML_METHOD(MLStringOfMethod, KiwiConstraintT) {
	kiwi_constraint_t *C = (kiwi_constraint_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "%g", C->Value.expression().constant());
	for (auto Term : C->Value.expression().terms()) {
		if (Term.coefficient() < 0.0) {
			ml_stringbuffer_addf(Buffer, " - %g", -Term.coefficient());
		} else {
			ml_stringbuffer_addf(Buffer, " + %g", Term.coefficient());
		}
		ml_stringbuffer_addf(Buffer, "%s", Term.variable().name().c_str());
	}
	switch (C->Value.op()) {
	case kiwi::RelationalOperator::OP_LE:
		ml_stringbuffer_add(Buffer, " <= 0", 5);
		break;
	case kiwi::RelationalOperator::OP_GE:
		ml_stringbuffer_add(Buffer, " >= 0", 5);
		break;
	case kiwi::RelationalOperator::OP_EQ:
		ml_stringbuffer_add(Buffer, " == 0", 5);
		break;
	}
	ml_stringbuffer_addf(Buffer, " @ %f", C->Value.strength());
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD("-", KiwiVariableT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	return (ml_value_t *)new kiwi_expression_t(-V->Value);
}

ML_METHOD("+", KiwiVariableT, MLNumberT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value + N);
}

ML_METHOD("+", MLNumberT, KiwiVariableT) {
	double N = ml_real_value(Args[0]);
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N + V->Value);
}

ML_METHOD("-", KiwiVariableT, MLNumberT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value - N);
}

ML_METHOD("-", MLNumberT, KiwiVariableT) {
	double N = ml_real_value(Args[0]);
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N - V->Value);
}

ML_METHOD("*", KiwiVariableT, MLNumberT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value * N);
}

ML_METHOD("*", MLNumberT, KiwiVariableT) {
	double N = ml_real_value(Args[0]);
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N * V->Value);
}

ML_METHOD("/", KiwiVariableT, MLNumberT) {
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value / N);
}

ML_METHOD("-", KiwiExpressionT) {
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0];
	return (ml_value_t *)new kiwi_expression_t(-V->Value);
}

ML_METHOD("+", KiwiExpressionT, MLNumberT) {
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value + N);
}

ML_METHOD("+", MLNumberT, KiwiExpressionT) {
	double N = ml_real_value(Args[0]);
	kiwi_expression_t *V = (kiwi_expression_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N + V->Value);
}

ML_METHOD("-", KiwiExpressionT, MLNumberT) {
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value - N);
}

ML_METHOD("-", MLNumberT, KiwiExpressionT) {
	double N = ml_real_value(Args[0]);
	kiwi_expression_t *V = (kiwi_expression_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N - V->Value);
}

ML_METHOD("*", KiwiExpressionT, MLNumberT) {
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value * N);
}

ML_METHOD("*", MLNumberT, KiwiExpressionT) {
	double N = ml_real_value(Args[0]);
	kiwi_expression_t *V = (kiwi_expression_t *)Args[1];
	return (ml_value_t *)new kiwi_expression_t(N * V->Value);
}

ML_METHOD("/", KiwiExpressionT, MLNumberT) {
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_expression_t(V->Value / N);
}

#define TYPED_ARITHMETIC_METHOD(NAME, OP, ATYPE1, CTYPE1, ATYPE2, CTYPE2) \
ML_METHOD(NAME, ATYPE1, ATYPE2) { \
	CTYPE1 *V1 = (CTYPE1 *)Args[0]; \
	CTYPE2 *V2 = (CTYPE2 *)Args[1]; \
	return (ml_value_t *)new kiwi_expression_t(V1->Value OP V2->Value); \
}

TYPED_ARITHMETIC_METHOD("+", +, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_expression_t);
TYPED_ARITHMETIC_METHOD("+", +, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_expression_t);
TYPED_ARITHMETIC_METHOD("+", +, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_expression_t);
TYPED_ARITHMETIC_METHOD("+", +, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_expression_t);

#define ARITHMETIC_METHODS(NAME, OP) \
TYPED_ARITHMETIC_METHOD(NAME, OP, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_variable_t); \
\
TYPED_ARITHMETIC_METHOD(NAME, OP, KiwiVariableT, kiwi_variable_t, KiwiExpressionT, kiwi_expression_t); \
\
TYPED_ARITHMETIC_METHOD(NAME, OP, KiwiExpressionT, kiwi_expression_t, KiwiVariableT, kiwi_variable_t); \
\
TYPED_ARITHMETIC_METHOD(NAME, OP, KiwiExpressionT, kiwi_expression_t, KiwiExpressionT, kiwi_expression_t);

ARITHMETIC_METHODS("+", +);
ARITHMETIC_METHODS("-", -);

#define TYPED_COMPARISON_METHOD(NAME, OP, ATYPE1, CTYPE1, ATYPE2, CTYPE2) \
ML_METHOD(NAME, ATYPE1, ATYPE2) { \
	CTYPE1 *V1 = (CTYPE1 *)Args[0]; \
	CTYPE2 *V2 = (CTYPE2 *)Args[1]; \
	return (ml_value_t *)new kiwi_constraint_t(V1->Value OP V2->Value); \
}

#define COMPARISON_METHODS(NAME, OP) \
ML_METHOD(NAME, KiwiVariableT, MLNumberT) { \
	kiwi_variable_t *V = (kiwi_variable_t *)Args[0]; \
	double N = ml_real_value(Args[1]); \
	return (ml_value_t *)new kiwi_constraint_t(V->Value OP N); \
} \
\
ML_METHOD(NAME, MLNumberT, KiwiVariableT) { \
	double N = ml_real_value(Args[0]); \
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1]; \
	return (ml_value_t *)new kiwi_constraint_t(V->Value OP N); \
} \
\
ML_METHOD(NAME, KiwiExpressionT, MLNumberT) { \
	kiwi_expression_t *V = (kiwi_expression_t *)Args[0]; \
	double N = ml_real_value(Args[1]); \
	return (ml_value_t *)new kiwi_constraint_t(V->Value OP N); \
} \
\
ML_METHOD(NAME, MLNumberT, KiwiExpressionT) { \
	double N = ml_real_value(Args[0]); \
	kiwi_expression_t *V = (kiwi_expression_t *)Args[1]; \
	return (ml_value_t *)new kiwi_constraint_t(V->Value OP N); \
} \
\
TYPED_COMPARISON_METHOD(NAME, OP, KiwiVariableT, kiwi_variable_t, KiwiVariableT, kiwi_variable_t) \
\
TYPED_COMPARISON_METHOD(NAME, OP, KiwiVariableT, kiwi_variable_t, KiwiExpressionT, kiwi_expression_t) \
\
TYPED_COMPARISON_METHOD(NAME, OP, KiwiExpressionT, kiwi_expression_t, KiwiVariableT, kiwi_variable_t) \
\
TYPED_COMPARISON_METHOD(NAME, OP, KiwiExpressionT, kiwi_expression_t, KiwiExpressionT, kiwi_expression_t)

COMPARISON_METHODS("=", ==);
COMPARISON_METHODS("<=", <=);
COMPARISON_METHODS(">=", >=);

ML_METHOD("@", KiwiConstraintT, MLNumberT) {
	kiwi_constraint_t *V = (kiwi_constraint_t *)Args[0];
	double N = ml_real_value(Args[1]);
	return (ml_value_t *)new kiwi_constraint_t(V->Value | N);
}

ML_TYPE(KiwiSolverT, (), "kiwi-solver");

struct kiwi_solver_t : public gc {
	const ml_type_t *Type = KiwiSolverT;
	kiwi::Solver Value;
};

ML_FUNCTION(KiwiSolver) {
	return (ml_value_t *)new kiwi_solver_t();
}

ML_METHOD("add", KiwiSolverT, KiwiConstraintT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	kiwi_constraint_t *C = (kiwi_constraint_t *)Args[1];
	try {
		S->Value.addConstraint(C->Value);
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("remove", KiwiSolverT, KiwiConstraintT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	kiwi_constraint_t *C = (kiwi_constraint_t *)Args[1];
	try {
		S->Value.removeConstraint(C->Value);
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("add", KiwiSolverT, KiwiVariableT, MLNumberT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	try {
		S->Value.addEditVariable(V->Value, ml_real_value(Args[2]));
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("remove", KiwiSolverT, KiwiVariableT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	try {
		S->Value.removeEditVariable(V->Value);
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("suggest", KiwiSolverT, KiwiVariableT, MLNumberT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	kiwi_variable_t *V = (kiwi_variable_t *)Args[1];
	try {
		S->Value.suggestValue(V->Value, ml_real_value(Args[2]));
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("update", KiwiSolverT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	try {
		S->Value.updateVariables();
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

ML_METHOD("reset", KiwiSolverT) {
	kiwi_solver_t *S = (kiwi_solver_t *)Args[0];
	try {
		S->Value.reset();
	} catch (std::exception &Exception) {
		return ml_error("KiwiError", Exception.what());
	}
	return Args[0];
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
#include "ml_kiwi_init.cpp"
	stringmap_insert(KiwiVariableT->Exports, "of", KiwiVariable);
	ml_module_export(Module, "variable", (ml_value_t *)KiwiVariableT);
	stringmap_insert(KiwiSolverT->Exports, "of", KiwiSolver);
	ml_module_export(Module, "solver", (ml_value_t *)KiwiSolverT);
}
