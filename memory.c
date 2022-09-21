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
  node->size = 0; // start counting at 0 so that we may have 8 :)
  node->type = type;
  node->mark = 0;
  node->element = 1;
  node->next = 0;
  node->value.u32 = value;

  return node;
}

