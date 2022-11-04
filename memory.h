#ifndef MEMORY_H
#define MEMORY_H

#include "node.h"

extern Node * memory;
extern uintptr_t memsize;

extern Node * freelist;

void init_node_memory();
Node * copy(Node * node, int n_recurse);

/**
 * Allocate un-initialized node space at end of memory.
 */
Node * allocate_node();

/**
 * Initialize a fixed-sized node, either from
 * reclaimed memory or fully new.
 */
Node * new_node(Type type, uint32_t value);

/**
 * Return a stretchable node at end of memory.
 */
Node * new_array_node(Type type, uint32_t value);


/**
 * Try to relocate the given node into an existing free slot,
 * releasing the memory for the given node upon success.
 * Returns the node in its final location.
 */
Node * retrofit (Node * node);

/**
 * If the node is not yet an element,
 * make an element copy.
 */
Node * element(Node * node);

//
// 'env' functions put here
//
Node * lookup(Node * env, Node * name);
Node * dereference(Node * env, Node * name);
Node * find_macro(Node * env, Node * name);



// NIL == &memory[0]
#define NIL memory

#define index(node) ((node) - memory)
#define pointer(idxval) (&memory[idxval])

#define num_value_nodes(node) ((node->value.u32+7) / 8)
#endif /* MEMORY_H */
