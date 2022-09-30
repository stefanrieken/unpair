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

/**
 * If the node is not yet an element,
 * make an element copy.
 */
Node * element(Node * node);

// NIL == &memory[0]
#define NIL memory
#endif /* MEMORY_H */
