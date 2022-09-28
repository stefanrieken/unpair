#ifndef MEMORY_H
#define MEMORY_H

#include "node.h"

extern Node * memory;
extern uintptr_t memsize;

#define chunksize 64

void init_node_memory();
Node * allocate_node();
Node * new_node(Type type, uint32_t value);
Node * copy(Node * node, int n_recurse);

#endif /* MEMORY_H */
