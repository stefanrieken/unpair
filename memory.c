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

#define chunksize 1024
void init_node_memory()
{
  memory = malloc(sizeof(Node) * chunksize);
  memsize = 0;
}

Node * allocate_node()
{
  if(memsize%chunksize == 0)
  {
    Node * mem_old = memory;
    memory = realloc(memory, sizeof(Node) * (memsize+chunksize));
    if (mem_old != memory) printf("WARN: memory moved (memsize=%ld). This invalidates all pointers, which we presently can't handle!\n", memsize);
  }
  Node * node = &memory[memsize];
  memsize++;
  return node;
}

Node * new_node(Type type, uint32_t value)
{
  Node * node = allocate_node();
  node->array = false;
  node->type = type;
  node->mark = false;
  node->element = true;
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
  if (node->array) num_nodes = (node->value.u32 / sizeof(Node)) + 1;
  for (int i=0; i< num_nodes; i++)
    allocate_node();

  memcpy(result, node, sizeof(Node) * (num_nodes+1));

  if (n_recurse > 0 && node->next != 0)
    result->next = copy(&memory[node->next], n_recurse-1) - memory;
  else
    result->next = 0;

  return result;
}

/**
 * If the node is not yet an element,
 * make an element copy.
 */
Node * element(Node * node)
{
  if (!node->element)
  {
    node = copy(node, 0);
    node->element = true;
    node->next = 0;
  }
  return node;
}

