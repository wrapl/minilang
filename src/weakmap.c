#include "weakmap.h"
#include "ml_macros.h"
#include "minilang.h"
#include <stdio.h>
#include <string.h>

struct weakmap_node_t {
	const char *Key;
	void *Value;
	uint32_t Hash, Offset, Length;
};

static inline uint32_t murmur3_scramble(uint32_t K) {
	K *= 0xcc9e2d51;
	K = (K << 15) | (K >> 17);
	K *= 0x1b873593;
	return K;
}

static inline uint32_t weakmap_hash(const char *Key, int Length) {
	/*size_t Hash = 5381;
	for (const unsigned char *P = (const unsigned char *)Key; --Length >= 0; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;*/
	/*size_t Hash = 5381;
	int32_t *P = (int32_t *)Key;
	int I = Length;
	while (I > 4) {
		uint32_t K = *P++ + 4;
		Hash ^= murmur3_scramble(K);
		Hash = (Hash << 13) | (Hash >> 19);
		Hash = Hash * 5 + 0xe6546b64;
		I -= 4;
	}
	const unsigned char *P2 = (const unsigned char *)P;
	uint32_t K = 0;
	while (I > 0) {
		K <<= 8;
		K |= *P2++;
		--I;
	}
	Hash ^= murmur3_scramble(K);
	Hash ^= Length;
	Hash ^= Hash >> 16;
	Hash *= 0x85ebca6b;
	Hash ^= Hash >> 13;
	Hash *= 0xc2b2ae35;
	Hash ^= Hash >> 16;
	return Hash;*/
	uint32_t Hash = 5381;
	int32_t *P = (int32_t *)Key;
	int I = Length;
	while (I > 4) {
		uint32_t K = *P++ + 4;
		uint64_t K2 = (K * 0xcc9e2d51);
		uint32_t K3 = (K2 & 0xFFFFFFFF) ^ (K2 >> 32);
		Hash ^= K3;
		Hash = (Hash << 13) | (Hash >> 19);
		I -= 4;
	}
	const unsigned char *P2 = (const unsigned char *)P;
	uint32_t K = 0;
	while (I > 0) {
		K <<= 8;
		K |= *P2++;
		--I;
	}
	uint64_t K2 = (K * 0xcc9e2d51);
	uint32_t K3 = (K2 & 0xFFFFFFFF) ^ (K2 >> 32);
	Hash ^= K3;
	return Hash;
}

static inline const char *weakmap_copy_key(const char *Key, int Length) {
	char *Copy = GC_malloc_atomic(Length + 1);
	memcpy(Copy, Key, Length);
	Copy[Length] = 0;
	return Copy;
}

static __attribute__ ((noinline)) void *weakmap_grow(weakmap_t *Map, size_t Size) {
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
		Insert.Length = Old->Length;
		Insert.Value = Old->Value;
		Insert.Offset = 1;
		GC_unregister_disappearing_link(&Old->Value);
		size_t Index = Insert.Hash & Mask;
		weakmap_node_t *Node = Nodes + Index;
		while (Node->Value) {
			if (Node->Offset < Insert.Offset) {
				weakmap_node_t Next = *Node;
				GC_unregister_disappearing_link(&Node->Value);
				*Node = Insert;
				GC_general_register_disappearing_link(&Node->Value, Node->Value);
				Insert = Next;
			}
			Index = (Index + 1) & Mask;
			Node = Nodes + Index;
			Insert.Offset++;
		}
		//if (!Node->Key)
		--Space;
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

int weakmap_check(weakmap_t *Map) {
#ifdef ML_HOSTTHREADS
	pthread_mutex_lock(Map->Lock);
#endif
	int Corrupted = 0;
	weakmap_node_t *Node = Map->Nodes;
	if (Node) for (int I = Map->Mask + 1; --I >= 0; ++Node) {
		if (Node->Value) {
			if (weakmap_hash(Node->Key, strlen(Node->Key)) != Node->Hash) {
				Corrupted = 1;
				fprintf(stderr, "Weakmap corrupted\n");
				void *Base = GC_base((void *)(Node->Key - 8));
				if (Base) fprintf(stderr, "\tPrevious block = %ld\n", GC_size(Base));
			}
		}
	}
#ifdef ML_HOSTTHREADS
	pthread_mutex_unlock(Map->Lock);
#endif
	return Corrupted;
}

#define INIT_SIZE 64
#define MIN_SPACE 8

void weakmap_alloc(weakmap_t *Map) {
	weakmap_node_t *Nodes = Map->Nodes = GC_malloc_atomic(INIT_SIZE * sizeof(weakmap_node_t));
	memset(Nodes, 0, INIT_SIZE * sizeof(weakmap_node_t));
	Map->Mask = INIT_SIZE - 1;
	Map->Space = INIT_SIZE;
}

static void *weakmap_node_value(void *Node) {
	return ((weakmap_node_t *)Node)->Value;
}

void *weakmap_insert(weakmap_t *Map, const char *Key, int Length, void *(*missing)(const char *, int)) {
	size_t Hash = weakmap_hash(Key, Length);
#ifdef ML_HOSTTHREADS
	pthread_mutex_lock(Map->Lock);
	//GC_alloc_lock();
#endif
	weakmap_node_t *Nodes = Map->Nodes;
	size_t Mask = Map->Mask;
	size_t Index = Hash & Mask;
	//fprintf(stderr, "Looking for key %.*s @ %d into weakmap\n", Length, Key, Index);
	size_t Offset = 1;
	weakmap_node_t *Node = Nodes + Index;
	while (Offset <= Node->Offset) {
		//if (Node->Key) fprintf(stderr, "[%d] -> %s +%d\n", Index, Node->Key, Node->Offset);
		if (Node->Hash == Hash && Node->Length == Length) {
			//void *Value = Node->Value;
			void *Value = GC_call_with_alloc_lock(weakmap_node_value, Node);
			//if (!Value) fprintf(stderr, "Value was deleted: (%s)\n", Key);
			if (Value && !memcmp(Node->Key, Key, Length)) {
#ifdef ML_HOSTTHREADS
				pthread_mutex_unlock(Map->Lock);
				//GC_alloc_unlock();
#endif
				return Value;
			}
		}
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
		Offset++;
	}
	/*if (Map->Space <= MIN_SPACE) {
		size_t Size = Mask + 1;
		//weakmap_grow(Map, Map->Deleted > MIN_SPACE ? Size : (Size * 2));
		weakmap_grow(Map, Size * 2);
		Nodes = Map->Nodes;
		Mask = Map->Mask;
		Index = Hash & Mask;
		Node = Nodes + Index;
	}*/


	size_t Size = Mask + 1;
	size_t MinSpace = Size >> 2;
	//fprintf(stderr, "Space = %d, MinSpace = %d\n", Map->Space, MinSpace);
	//fprintf(stderr, "Nodes = %ld\n", Nodes);
	if (Map->Space <= MinSpace) {
		size_t ActualSpace = 0;
		for (weakmap_node_t *Node = Map->Nodes, *Limit = Node + Size; Node < Limit; ++Node) {
			if (!Node->Value) ++ActualSpace;
		}
		weakmap_grow(Map, ActualSpace > MinSpace ? Size : (Size * 2));
		//weakmap_grow(Map, Size * 2);
		Nodes = Map->Nodes;
		Mask = Map->Mask;
		Index = Hash & Mask;
		Node = Nodes + Index;
		Offset = 1;
	}
	//fprintf(stderr, "Nodes = %ld\n", Nodes);
	weakmap_node_t Insert;
	Insert.Key = weakmap_copy_key(Key, Length);
	Insert.Hash = Hash;
	Insert.Length = Length;
	Insert.Offset = Offset;
	void *Result = Insert.Value = missing(Insert.Key, Length);
	//fprintf(stderr, "Creating missing value for key %s: space %ld ->", Insert.Key, Map->Space);
	//GC_register_finalizer(Result, (GC_finalization_proc)weakmap_delete, Map, NULL, NULL);
	//fprintf(stderr, "Inserting key %s @ %d +%d into weakmap\n", Insert.Key, Index, Insert.Offset);
	while (Node->Value) {
		if (Node->Offset < Insert.Offset) {
			//fprintf(stderr, "Moving key %s @ %d +%d into weakmap\n", Node->Key, Index, Node->Offset);
			weakmap_node_t Next = *Node;
			GC_unregister_disappearing_link(&Node->Value);
			*Node = Insert;
			GC_general_register_disappearing_link(&Node->Value, Node->Value);
			Insert = Next;
		}
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
		Insert.Offset++;
	}
	//if (!Node->Key)
	--Map->Space;
	*Node = Insert;
	GC_general_register_disappearing_link(&Node->Value, Node->Value);
	//fprintf(stderr, "%ld\n", Map->Space);
#ifdef ML_HOSTTHREADS
	pthread_mutex_unlock(Map->Lock);
	//GC_alloc_unlock();
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
