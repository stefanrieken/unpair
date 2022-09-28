#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "node.h"

//
// MEMORY
//

/**
 * We must do at least a bit of our own memory management
 * on nodes, so that we may also GC them.
 */
Node * memory;
uintptr_t memsize;

#define chunksize 64
void init_node_memory()
{
  memory = malloc(sizeof(Node) * chunksize);
  memsize = 0;
}

Node * allocate_node()
{
  if(memsize%chunksize == 0)
  {
    memory = malloc(sizeof(Node) * (memsize+chunksize));
  }
  Node * node = &memory[memsize];
  memsize++;
  return node;
}

Node * new_node(Type type, uint32_t value)
{
  Node * node = allocate_node();
  node->array = 0;
  node->type = type;
  node->mark = 0;
  node->element = 1;
  node->next = 0;
  node->value.u32 = value;

  return node;
}

/**
 * n_recurse:
 * - 0 = none
 * - n = n times
 * - -n = indefinitely (or until int wraps around...)
 */
Node * copy(Node * node, int n_recurse)
{
  if (node == NULL) return NULL;
  Node * result = new_node(node->type, node->value.u32);
  // Allocate any overflow nodes
  int num_nodes = 0;
  if (node->array) num_nodes = (node->value.u32 / 8) + 1;
  for (int i=0; i< num_nodes; i++)
    allocate_node();

  memcpy(result, node, sizeof(Node) * (num_nodes+1));

  if (n_recurse > 0 && node->next != 0)
    result->next = copy(&memory[node->next], n_recurse-1) - memory;

  return result;
}

