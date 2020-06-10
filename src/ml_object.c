#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_object.h"

typedef struct ml_class_t ml_class_t;
typedef struct ml_object_t ml_object_t;

struct ml_class_t {
	ml_type_t Base;
	int NumFields;
	ml_value_t *Fields[];
};

struct ml_object_t {
	const ml_type_t *Type;
	ml_value_t *Fields[];
};

ML_TYPE(MLObjectT, (), "object");

static void ml_class_call(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = stringmap_search(Class->Base.Exports, "of");
	return Constructor->Type->call(Caller, Constructor, Count, Args);
}

ML_TYPE(MLClassT, (MLTypeT), "class",
	.call = (void *)ml_class_call
);

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLObjectT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_value_t *String = ml_inline(MLStringOfMethod, 1, Args[1]);
	if (String->Type == MLStringT) {
		ml_stringbuffer_add(Buffer, ml_string_value(String), ml_string_length(String));
		return MLSome;
	} else if (String->Type == MLErrorT) {
		return String;
	} else {
		return ml_error("ResultError", "string method did not return string");
	}
}

ML_METHOD(MLStringOfMethod, MLObjectT) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	ml_class_t *Class = (ml_class_t *)Object->Type;
	if (Class->NumFields > 0) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_addf(Buffer, "%s(", Class->Base.Name);
		const char *Name = ml_method_name(Class->Fields[0]);
		ml_stringbuffer_add(Buffer, Name, strlen(Name));
		ml_stringbuffer_add(Buffer, ": ", 2);
		ml_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[0]);
		for (int I = 1; I < Class->NumFields; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			const char *Name = ml_method_name(Class->Fields[I]);
			ml_stringbuffer_add(Buffer, Name, strlen(Name));
			ml_stringbuffer_add(Buffer, ": ", 2);
			ml_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[I]);
		}
		ml_stringbuffer_add(Buffer, ")", 1);
		return ml_stringbuffer_get_string(Buffer);
	} else {
		return ml_string_format("%s()", Class->Base.Name);
	}
}

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	int Index = (char *)Data - (char *)0;
	ml_object_t *Object = (ml_object_t *)Args[0];
	return ml_reference((ml_value_t **)((char *)Object + Index));
}

static void ml_constructor_fn(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields, ml_value_t *);
	Object->Type = (ml_type_t *)Class;
	ml_value_t **Slot = Object->Fields;
	for (int I = Class->NumFields; --I >= 0; ++Slot) *Slot = MLNil;
	// TODO: If Class has an "init" function, call that instead
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_RETURN(Arg);
		if (Arg->Type == MLNamesT) {
			ML_NAMES_FOREACH(Args[I], Node) {
				++I;
				ml_value_t *Field = Node->Value;
				for (int J = 0; J < Class->NumFields; ++J) {
					if (Class->Fields[J] == Field) {
						ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
						if (Arg->Type == MLErrorT) ML_RETURN(Arg);
						Object->Fields[J] = Arg;
						goto found;
					}
				}
				ML_RETURN(ml_error("ValueError", "Class %s does not have field %s", Class->Base.Name, ml_method_name(Field)));
				found:;
			}
			break;
		} else if (I > Class->NumFields) {
			break;
		} else {
			Object->Fields[I] = Arg;
		}
	}
	ML_RETURN(Object);
}

ML_FUNCTION(MLClassNew) {
	static ml_value_t **FieldFns = NULL;
	static int NumFieldFns = 0;
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	int NumFields = 0, NumParents = 0;
	for (int I = 1; I < Count; ++I) {
		if (Args[I]->Type == MLMethodT) {
			++NumFields;
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			NumFields += Parent->NumFields;
			const ml_type_t **Types = Parent->Base.Types;
			do ++NumParents; while (*++Types != MLObjectT);
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			if (Parent->Rank == INT_MAX) return ml_error("TypeError", "Classes can not inherit from native types");
			const ml_type_t **Types = Parent->Types;
			do ++NumParents; while (*++Types);
		} else if (ml_is(Args[I], MLListT)) {
			ML_LIST_FOREACH(Args[I], Iter) {
				if (Iter->Value->Type == MLMethodT) {
					++NumFields;
				} else if (ml_is(Iter->Value, MLClassT)) {
					ml_class_t *Parent = (ml_class_t *)Iter->Value;
					NumFields += Parent->NumFields;
					const ml_type_t **Types = Parent->Base.Types;
					do ++NumParents; while (*++Types != MLObjectT);
				} else if (ml_is(Args[I], MLTypeT)) {
					ml_type_t *Parent = (ml_type_t *)Iter->Value;
					if (Parent->Rank == INT_MAX) return ml_error("TypeError", "Classes can not inherit from native types");
					const ml_type_t **Types = Parent->Types;
					do ++NumParents; while (*++Types);
				}
			}
		} else if (ml_is(Args[I], MLMapT)) {
		} else {
			return ml_error("TypeError", "Unexpected argument type: <%s>", Args[I]->Type->Name);
		}
	}
	ml_class_t *Class = xnew(ml_class_t, NumFields, ml_value_t *);
	Class->Base.Type = MLClassT;
	Class->Base.Name = Name;
	Class->Base.hash = ml_default_hash;
	Class->Base.call = ml_default_call;
	Class->Base.deref = ml_default_deref;
	Class->Base.assign = ml_default_assign;
	const ml_type_t **Parents = Class->Base.Types = anew(const ml_type_t *, NumParents + 4);
	*Parents++ = (ml_type_t *)Class;
	Class->NumFields = NumFields;
	ml_value_t **Fields = Class->Fields;
	ml_value_t *Constructor = ml_functionx(Class, (void *)ml_constructor_fn);
	stringmap_insert(Class->Base.Exports, "of", Constructor);
	for (int I = 1; I < Count; ++I) {
		if (Args[I]->Type == MLMethodT) {
			*Fields++ = Args[I];
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			for (int I = 0; I < Parent->NumFields; ++I) *Fields++ = Parent->Fields[I];
			const ml_type_t **Types = Parent->Base.Types;
			while (*Types != MLObjectT) *Parents++ = *Types++;
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			const ml_type_t **Types = Parent->Types;
			while (*Types) *Parents++ = *Types++;
		} else if (ml_is(Args[I], MLListT)) {
			ML_LIST_FOREACH(Args[I], Iter) {
				if (Iter->Value->Type == MLMethodT) {
					*Fields++ = Iter->Value;
				} else if (ml_is(Iter->Value, MLClassT)) {
					ml_class_t *Parent = (ml_class_t *)Iter->Value;
					for (int I = 0; I < Parent->NumFields; ++I) *Fields++ = Parent->Fields[I];
					const ml_type_t **Types = Parent->Base.Types;
					while (*Types != MLObjectT) *Parents++ = *Types++;
				} else if (ml_is(Iter->Value, MLTypeT)) {
					ml_type_t *Parent = (ml_type_t *)Iter->Value;
					const ml_type_t **Types = Parent->Types;
					while (*Types) *Parents++ = *Types++;
				}
			}
		} else if (ml_is(Args[I], MLMapT)) {
			ML_MAP_FOREACH(Args[I], Iter) {
				ml_value_t *Key = Iter->Key;
				if (Key->Type != MLStringT) return ml_error("TypeError", "Class field name must be a string");
				stringmap_insert(Class->Base.Exports, ml_string_value(Key), Iter->Value);
			}
		}
	}
	*Parents++ = MLObjectT;
	*Parents++ = MLAnyT;
	if (Class->NumFields > NumFieldFns) {
		ml_value_t **NewFieldFns = anew(ml_value_t *, Class->NumFields);
		memcpy(NewFieldFns, FieldFns, NumFieldFns * sizeof(ml_value_t *));
		for (int I = NumFieldFns; I < Class->NumFields; ++I) {
			NewFieldFns[I] = ml_function(((ml_object_t *)0)->Fields + I, ml_field_fn);
		}
		FieldFns = NewFieldFns;
		NumFieldFns = Class->NumFields;
	}
	for (int I = 0; I < Class->NumFields; ++I) {
		ml_method_by_array(Class->Fields[I], FieldFns[I], 1, (ml_type_t **)&Class);
	}
	stringmap_insert(Class->Base.Exports, "new", Constructor);
	return (ml_value_t *)Class;
}

ML_FUNCTION(MLMethodSet) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLMethodT);
	for (int I = 1; I < Count - 1; ++I) {
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLTypeT);
	}
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_method_by_array(Args[0], Args[Count - 1], Count - 2, (ml_type_t **)(Args + 1));
	return Args[Count - 1];
}

typedef struct ml_assignable_t {
	const ml_type_t *Type;
	ml_value_t *Get, *Set;
} ml_assignable_t;

static ml_value_t *ml_assignable_deref(ml_assignable_t *Assignable) {
	return ml_call(Assignable->Get, 0, NULL);
}

static ml_value_t *ml_assignable_assign(ml_assignable_t *Assignable, ml_value_t *Value) {
	return ml_call(Assignable->Set, 1, &Value);
}

ML_TYPE(MLAssignableT, (), "assignable",
	.deref = (void *)ml_assignable_deref,
	.assign = (void *)ml_assignable_assign
);

ML_FUNCTION(MLProperty) {
	ML_CHECK_ARG_COUNT(2);
	ml_assignable_t *Assignable = new(ml_assignable_t);
	Assignable->Type = MLAssignableT;
	Assignable->Get = Args[0];
	Assignable->Set = Args[1];
	return (ml_value_t *)Assignable;
}

size_t ml_class_size(ml_value_t *Value) {
	return ((ml_class_t *)Value)->NumFields;
}

ml_value_t *ml_class_field(ml_value_t *Value, size_t Field) {
	return ((ml_class_t *)Value)->Fields[Field];
}

size_t ml_object_size(ml_value_t *Value) {
	return ((ml_class_t *)Value->Type)->NumFields;
}

ml_value_t *ml_object_field(ml_value_t *Value, size_t Field) {
	return ((ml_object_t *)Value)->Fields[Field];
}

void ml_object_init(stringmap_t *Globals) {
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	stringmap_insert(Globals, "property", MLProperty);
	stringmap_insert(Globals, "object", MLObjectT);
	stringmap_insert(Globals, "class", MLClassT);
	stringmap_insert(MLClassT->Exports, "of", MLClassNew);
	stringmap_insert(MLClassT->Exports, "new", MLClassNew);
#include "ml_object_init.c"
}
