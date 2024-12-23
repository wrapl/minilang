#include "weakmap.h"
#include "ml_macros.h"
#include "minilang.h"
#include <stdio.h>
#include <string.h>

struct weakmap_node_t {
	const char *Key;
	void *Value;
	size_t Hash, Offset;
};

static inline size_t weakmap_hash(const char *Key, int Length) {
	size_t Hash = 5381;
	for (const unsigned char *P = (const unsigned char *)Key; --Length >= 0; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static inline const char *weakmap_copy_key(const char *Key, int Length) {
	char *Copy = GC_malloc_atomic(Length + 1);
	memcpy(Copy, Key, Length);
	Copy[Length] = 0;
	return Copy;
}

static void *weakmap_grow(weakmap_t *Map, size_t Size) {
	//fprintf(stderr, "Growing map from %ld -> %ld\n", Map->Mask + 1, Size);
	weakmap_node_t *Nodes = GC_malloc_atomic(Size * sizeof(weakmap_node_t));
	memset(Nodes, 0, Size * sizeof(weakmap_node_t));
	weakmap_node_t *Old = Map->Nodes;
	size_t Mask = Size - 1;
	size_t Space = Size;
	for (int I = Map->Mask + 1; --I >= 0; ++Old) if (Old->Value) {
		weakmap_node_t Insert;
		Insert.Key = Old->Key;
		Insert.Hash = Old->Hash;
		Insert.Value = Old->Value;
		Insert.Offset = 0;
		size_t Index = Insert.Hash & Mask;
		weakmap_node_t *Node = Nodes + Index;
		while (Node->Value) {
			if (Node->Offset < Insert.Offset) {
				weakmap_node_t Next = *Node;
				*Node = Insert;
				GC_general_register_disappearing_link(&Node->Value, Node->Value);
				Insert = Next;
			}
			Index = (Index + 1) & Mask;
			Node = Nodes + Index;
			Insert.Offset++;
		}
		if (!Node->Key) --Space;
		*Node = Insert;
		GC_general_register_disappearing_link(&Node->Value, Node->Value);
	}
	//fprintf(stderr, "\t -> %ld\n", Map->Space);
	Map->Nodes = Nodes;
	Map->Mask = Mask;
	Map->Space = Space;
	//Map->Deleted = 0;
	return NULL;
}

/*static void weakmap_delete(void *Value, weakmap_t *Map) {
	++Map->Deleted;
	fprintf(stderr, "Finalizing value -> %ld\n", Map->Deleted);
}*/

static void *weakmap_value(weakmap_node_t *Node) {
	return Node->Value;
}

#define INIT_SIZE 64
#define MIN_SPACE 8

void *weakmap_insert(weakmap_t *Map, const char *Key, int Length, void *(*missing)(const char *, int)) {
	size_t Hash = weakmap_hash(Key, Length);
#ifdef ML_THREADSAFE
	pthread_mutex_lock(Map->Lock);
#endif
	if (!Map->Nodes) {
		weakmap_node_t *Nodes = Map->Nodes = GC_malloc_atomic(INIT_SIZE * sizeof(weakmap_node_t));
		memset(Nodes, 0, INIT_SIZE * sizeof(weakmap_node_t));
		size_t Index = Hash % INIT_SIZE;
		Map->Mask = Map->Space = INIT_SIZE - 1;
		//Map->Deleted = 0;
		weakmap_node_t *Node = Nodes + Index;
		Node->Key = weakmap_copy_key(Key, Length);
		Node->Hash = Hash;
		void *Result = Node->Value = missing(Node->Key, Length);
		GC_general_register_disappearing_link(&Node->Value, Result);
#ifdef ML_THREADSAFE
		pthread_mutex_unlock(Map->Lock);
#endif
		return Result;
	}
	weakmap_node_t *Nodes = Map->Nodes;
	size_t Mask = Map->Mask;
	size_t Index = Hash & Mask;
	size_t Offset = 0;
	weakmap_node_t *Node = Nodes + Index;
	while (Offset <= Node->Offset) {
		if (Node->Hash == Hash) {
			if (Node->Value && !strncmp(Node->Key, Key, Length)) {
#ifdef ML_THREADSAFE
				pthread_mutex_unlock(Map->Lock);
#endif
				return Node->Value;
			}
		}
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
		Offset++;
	}
	if (Map->Space <= MIN_SPACE) {
		size_t Size = Mask + 1;
		//weakmap_grow(Map, Map->Deleted > MIN_SPACE ? Size : (Size * 2));
		weakmap_grow(Map, Size * 2);
		Nodes = Map->Nodes;
		Mask = Map->Mask;
		Index = Hash & Mask;
		Node = Nodes + Index;
	}
	weakmap_node_t Insert;
	Insert.Hash = Hash;
	Insert.Key = weakmap_copy_key(Key, Length);
	Insert.Offset = Offset;
	void *Result = Insert.Value = missing(Insert.Key, Length);
	//fprintf(stderr, "Creating missing value for key %s: space %ld ->", Insert.Key, Map->Space);
	//GC_register_finalizer(Result, (GC_finalization_proc)weakmap_delete, Map, NULL, NULL);
	while (Node->Value) {
		if (Node->Offset < Insert.Offset) {
			weakmap_node_t Next = *Node;
			*Node = Insert;
			GC_general_register_disappearing_link(&Node->Value, Node->Value);
			Insert = Next;
		}
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
		Insert.Offset++;
	}
	if (!Node->Key) --Map->Space;
	*Node = Insert;
	GC_general_register_disappearing_link(&Node->Value, Node->Value);
	//fprintf(stderr, "%ld\n", Map->Space);
#ifdef ML_THREADSAFE
	pthread_mutex_unlock(Map->Lock);
#endif
	return Result;
}

int weakmap_foreach(weakmap_t *Map, void *Data, int (*callback)(const char *, void *, void *)) {
	weakmap_node_t *Node = Map->Nodes;
	for (int I = Map->Mask + 1; --I >= 0; ++Node) {
		void *Value = GC_call_with_alloc_lock((GC_fn_type)weakmap_value, Node);
		if (Value) if (callback(Node->Key, Value, Data)) return 1;
	}
	return 0;
}

typedef struct {
	ml_type_t *Type;
	const char *Name;
	size_t Value;
} ml_weakmap_token_t;

ML_TYPE(WeakMapTokenT, (), "weakmap::token");

ML_METHOD("append", MLStringBufferT, WeakMapTokenT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_weakmap_token_t *Token = (ml_weakmap_token_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "[%ld %s]", Token->Value, Token->Name);
	return MLSome;
}

static void *weak_map_token(const char *Name, int Length) {
	static size_t Last = 0;
	ml_weakmap_token_t *Token = new(ml_weakmap_token_t);
	Token->Type = WeakMapTokenT;
	Token->Name = Name;
	Token->Value = ++Last;
	return Token;
}

typedef struct {
	ml_type_t *Type;
	weakmap_t Map[1];
} ml_weakmap_t;

ML_TYPE(WeakMapT, (), "weakmap");

ML_METHOD(WeakMapT) {
	ml_weakmap_t *WeakMap = new(ml_weakmap_t);
	WeakMap->Type = WeakMapT;
	return (ml_value_t *)WeakMap;
}

ML_METHOD("insert", WeakMapT, MLStringT) {
	ml_weakmap_t *WeakMap = (ml_weakmap_t *)Args[0];
	const char *Key = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	return (ml_value_t *)weakmap_insert(WeakMap->Map, Key, Length, weak_map_token);
}

void weakmap_init(stringmap_t *Globals) {
#include "weakmap_init.c"
	if (Globals) {
		stringmap_insert(Globals, "weakmap", WeakMapT);
	}
}
