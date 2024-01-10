#include "ml_macros.h"
#include "minilang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	const char *Key, *Used;
	size_t Hash, Offset;
} ml_unique_node_t;

static struct {
	ml_unique_node_t *Nodes;
	size_t Mask, Space;
} UniqueMap[1] = {{NULL, 0, 0}};

static inline size_t ml_unique_hash(const char *Key, int Length) {
	//if (Length >= sizeof(size_t)) return *(size_t *)Key;
	size_t Hash = 5381;
	for (const unsigned char *P = (const unsigned char *)Key; --Length >= 0; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static inline const char *ml_unique_copy_key(const char *Key, int Length) {
	char *Copy = GC_malloc_atomic(Length + 1);
	memcpy(Copy, Key, Length);
	Copy[Length] = 0;
	return Copy;
}

static void ml_unique_grow(size_t Size) {
	fprintf(stderr, "Growing map from %ld -> %ld\n", UniqueMap->Mask + 1, Size);
	ml_unique_node_t *Nodes = GC_malloc/*_atomic*/(Size * sizeof(ml_unique_node_t));
	memset(Nodes, 0, Size * sizeof(ml_unique_node_t));
	ml_unique_node_t *Old = UniqueMap->Nodes;
	size_t Mask = Size - 1;
	size_t Space = Size;
	for (int I = UniqueMap->Mask + 1; --I >= 0; ++Old) if (Old->Key) {
		--Space;
		ml_unique_node_t Insert;
		Insert.Key = Old->Key;
		Insert.Used = "Used";
		Insert.Hash = Old->Hash;
		Insert.Offset = 0;
		size_t Index = Insert.Hash & Mask;
		ml_unique_node_t *Node = Nodes + Index;
		while (Node->Key) {
			if (Node->Offset < Insert.Offset) {
				ml_unique_node_t Next = *Node;
				*Node = Insert;
				//GC_general_register_disappearing_link((void **)&Node->Key, Node->Key);
				Insert = Next;
			}
			Insert.Offset++;
			Index = (Index + 1) & Mask;
			Node = Nodes + Index;
		}
		*Node = Insert;
		//GC_general_register_disappearing_link((void **)&Node->Key, Node->Key);
	}
	fprintf(stderr, "\t -> %ld\n", UniqueMap->Space);
	UniqueMap->Nodes = Nodes;
	UniqueMap->Mask = Mask;
	UniqueMap->Space = Space;
}

#define INIT_SIZE 64
#define MIN_SPACE 8

static void ml_unique_cleanup() {
	// TODO: shift later nodes down and increase Map->Space instead of copying
	ml_unique_node_t *Node = UniqueMap->Nodes;
	size_t Deleted = 0;
	for (int I = UniqueMap->Mask + 1; --I >= 0; ++Node) if (!Node->Key) {
		if (++Deleted > MIN_SPACE) return ml_unique_grow(UniqueMap->Mask + 1);
	}
}

const char *ml_unique(const char *Key, int Length) {
	size_t Hash = ml_unique_hash(Key, Length);
	if (!UniqueMap->Nodes) {
		ml_unique_node_t *Nodes = UniqueMap->Nodes = GC_malloc/*_atomic*/(INIT_SIZE * sizeof(ml_unique_node_t));
		memset(Nodes, 0, INIT_SIZE * sizeof(ml_unique_node_t));
		size_t Index = Hash % INIT_SIZE;
		UniqueMap->Mask = UniqueMap->Space = INIT_SIZE - 1;
		ml_unique_node_t *Node = Nodes + Index;
		const char *Result = Node->Key = ml_unique_copy_key(Key, Length);
		Node->Used = "Used";
		Node->Hash = Hash;
		//GC_general_register_disappearing_link((void **)&Node->Key, Node->Key);
		return Result;
	}
	ml_unique_node_t *Nodes = UniqueMap->Nodes;
	size_t Mask = UniqueMap->Mask;
	size_t Index = Hash & Mask;
	size_t Offset = 0;
	ml_unique_node_t *Node = Nodes + Index;
	while (Offset <= Node->Offset) {
		if (Node->Hash == Hash) {
			if (Node->Key && !strncmp(Node->Key, Key, Length)) return Node->Key;
		}
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
		Offset++;
	}
	if (UniqueMap->Space <= MIN_SPACE) ml_unique_cleanup();
	if (UniqueMap->Space <= MIN_SPACE) {
		size_t Size = Mask + 1;
		ml_unique_grow(Size * 2);
		Nodes = UniqueMap->Nodes;
		Mask = UniqueMap->Mask;
		Index = Hash & Mask;
		Node = Nodes + Index;
	}
	ml_unique_node_t Insert;
	Insert.Hash = Hash;
	const char *Result = Insert.Key = ml_unique_copy_key(Key, Length);
	Insert.Used = "Used";
	Insert.Offset = Offset;
	fprintf(stderr, "Creating missing value for key %s[%d] @ %ld: space %ld ->", Insert.Key, Length, Index, UniqueMap->Space);
	//if (!strcmp(Result, "buffer")) asm("int3");
	--UniqueMap->Space;
	while (Node->Used) {
		if (Node->Offset < Insert.Offset) {
			ml_unique_node_t Next = *Node;
			*Node = Insert;
			//GC_general_register_disappearing_link((void **)&Node->Key, Node->Key);
			Insert = Next;
		}
		Insert.Offset++;
		Index = (Index + 1) & Mask;
		Node = Nodes + Index;
	}
	*Node = Insert;
	//GC_general_register_disappearing_link((void **)&Node->Key, Node->Key);
	fprintf(stderr, "%ld\n", UniqueMap->Space);
	return Result;
}
