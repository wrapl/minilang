#include "stringmap2.h"
#include "ml_macros.h"
#include <string.h>
#include <stdint.h>

static uint32_t stringmap2_hash(const char *Key) {
	uint32_t Hash = 5381;
	for (const unsigned char *P = (const unsigned char *)Key; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

struct stringmap2_node_t {
	const char *Key;
	void *Value;
	uint32_t Hash, Offset;
};

stringmap2_t *stringmap2_new() {
	return new(stringmap2_t);
}

stringmap2_t *stringmap2_copy(stringmap2_t *Map) {
	if (!Map->Mask) return stringmap2_new();
	stringmap2_node_t *Nodes = anew(stringmap2_node_t, Map->Mask + 1);
	memcpy(Nodes, Map->Nodes, (Map->Mask + 1) * sizeof(stringmap2_node_t));
	stringmap2_t *Map2 = new(stringmap2_t);
	Map2->Nodes = Nodes;
	Map2->Mask = Map->Mask;
	Map2->Size = Map->Size;
	return Map2;
}

void *stringmap2_search(const stringmap2_t *Map, const char *Key) {
	uint32_t Hash = stringmap2_hash(Key);
	uint32_t Mask = Map->Mask;
	uint32_t Index = Hash & Mask;
	stringmap2_node_t *Nodes = Map->Nodes;
	uint32_t Offset = 0;
	for (;;) {
		stringmap2_node_t *Node = Nodes + Index;
		if (!Node->Value || Node->Offset > Offset) break;
		if (Node->Hash == Hash && !strcmp(Node->Key, Key)) return Node->Value;
		++Offset;
		Index = (Index + 1) & Mask;
	}
	return NULL;
}

static void stringmap2_grow(stringmap2_t *Map) {
	if (!Map->Mask) {
		Map->Nodes = anew(stringmap2_node_t, 4);
		Map->Mask = 3;
		return;
	}
	uint32_t Mask = 2 * Map->Mask + 1;
	stringmap2_node_t *Old = Map->Nodes;
	stringmap2_node_t *Nodes = anew(stringmap2_node_t, Mask + 1);
	for (int N = Map->Mask + 1; --N >= 0; ++Old) {
		if (!Old->Key) continue;
		uint32_t Index = Old->Hash & Mask;
		uint32_t Offset = 0;
		for (;;) {
			stringmap2_node_t *Node = Nodes + Index;
			if (!Node->Key) {
				Node->Key = Old->Key;
				Node->Value = Old->Value;
				Node->Hash = Old->Hash;
				Node->Offset = Offset;
				break;
			}
			if (Node->Offset > Offset) {
				stringmap2_node_t Temp = *Node;
				Node->Key = Old->Key;
				Node->Value = Old->Value;
				Node->Hash = Old->Hash;
				Node->Offset = Offset;
				Old->Key = Temp.Key;
				Old->Value = Temp.Value;
				Old->Hash = Temp.Hash;
				Offset = Temp.Offset;
			}
			Index = (Index + 1) & Mask;
			++Offset;
		}
	}
	Map->Nodes = Nodes;
	Map->Mask = Mask;
}

void **stringmap2_slot(stringmap2_t *Map, const char *Key) {
	if ((Map->Size + (Map->Size >> 2)) >= Map->Mask) stringmap2_grow(Map);
	uint32_t Hash = stringmap2_hash(Key);
	uint32_t Mask = Map->Mask;
	uint32_t Index = Hash & Mask;
	stringmap2_node_t *Nodes = Map->Nodes;
	uint32_t Offset = 0;
	void *Value, **Slot;
	for (;;) {
		stringmap2_node_t *Node = Nodes + Index;
		if (!Node->Key) {
			Node->Key = Key;
			Node->Hash = Hash;
			Node->Offset = Offset;
			++Map->Size;
			return &(Node->Value);
		}
		if (Node->Hash == Hash && !strcmp(Node->Key, Key)) {
			return &(Node->Value);
		}
		if (Node->Offset > Offset) {
			stringmap2_node_t Old = Nodes[Index];
			Nodes[Index].Key = Key;
			Slot = &Nodes[Index].Value;
			Nodes[Index].Hash = Hash;
			Nodes[Index].Offset = Offset;
			Key = Old.Key;
			Value = Old.Value;
			Hash = Old.Hash;
			Offset = Old.Offset;
			++Map->Size;
			break;
		}
		++Offset;
		Index = (Index + 1) & Mask;
	}
	for (;;) {
		++Offset;
		Index = (Index + 1) & Mask;
		stringmap2_node_t *Node = Nodes + Index;
		if (!Node->Key) {
			Node->Key = Key;
			Node->Value = Value;
			Node->Hash = Hash;
			Node->Offset = Offset;
			break;
		}
		if (Node->Offset >= Offset) {
			stringmap2_node_t Old = Nodes[Index];
			Nodes[Index].Key = Key;
			Nodes[Index].Value = Value;
			Nodes[Index].Hash = Hash;
			Nodes[Index].Offset = Offset;
			Key = Old.Key;
			Value = Old.Value;
			Hash = Old.Hash;
			Offset = Old.Offset;
		}
	}
	return Slot;
}

void *stringmap2_insert(stringmap2_t *Map, const char *Key, void *Value) {
	void **Slot = stringmap2_slot(Map, Key);
	void *Old = *Slot;
	*Slot = Value;
	return Old;
}

void *stringmap2_remove(stringmap2_t *Map, const char *Key) {
	uint32_t Hash = stringmap2_hash(Key);
	uint32_t Mask = Map->Mask;
	uint32_t Index = Hash & Mask;
	stringmap2_node_t *Nodes = Map->Nodes;
	uint32_t Offset = 0;
	void *Old = NULL;
	for (;;) {
		stringmap2_node_t *Node = Nodes + Index;
		if (!Node->Key) return NULL;
		if (Node->Hash == Hash && !strcmp(Node->Key, Key)) {
			Old = Node->Value;
			break;
		}
		if (Node->Offset > Offset) return NULL;
		++Offset;
		Index = (Index + 1) & Mask;
	}
	for (;;) {
		uint32_t Next = (Index + 1) & Mask;
		if (!Nodes[Next].Key || !Nodes[Next].Offset) {
			Nodes[Index].Key = NULL;
			Nodes[Index].Value = NULL;
			break;
		}
		Nodes[Index] = Nodes[Next];
		Offset = Nodes[Index].Offset;
		Nodes[Index].Offset = Offset - 1;
		Index = Next;
	}
	return Old;
}

int stringmap2_foreach(stringmap2_t *Map, void *Data, int (*callback)(const char *, void *, void *)) {
	stringmap2_node_t *Node = Map->Nodes;
	for (int N = Map->Mask + 1; --N >= 0; ++Node) {
		if (Node->Key) {
			int Stop = callback(Node->Key, Node->Value, Data);
			if (Stop) return Stop;
		}
	}
	return 0;
}
