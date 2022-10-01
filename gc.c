#include <stdbool.h>
#include <stdio.h>

#include "memory.h"
#include "gc.h"

int mark(Node * node)
{
  if(node->mark) return 0; // this should also check for NIL in practice

  node->mark = true;

  int marked = 1;
  if (node->array) marked += 1 + (node->value.u32 / sizeof(Node));

  if (node->type == TYPE_NODE
    || node->type == TYPE_FUNC
    || node->type == TYPE_VAR)
  {
    // These are all variants on
    // value field node pointers.
    marked += mark(&memory[node->value.u32]);
  }

  if (!node->element)
    marked += mark(&memory[node->next]);
  
  return marked;
}

Node * sweep(Node * freelist, uint32_t start)
{
  if (start >= memsize) return freelist;

  Node * current = &memory[start];

  // Memory debug output
  /*
  if (start % 8 == 0) printf("\n");
  printf("[%d%5.5s%2d]", current->mark, types[current->type], current->value.i32);

  if(current->array)
  {
    printf("[%s", strval(current));
    for(int i=0;i<sizeof(Node) - ((current->value.u32-1)%(sizeof (Node)));i++) printf(".");
    printf("]");
  }
  */

  if(current->mark)
    current->mark = false; // clear mark
  else
  {
    current->next = freelist - memory;
    freelist = current;
  }
  
  int skip = 1;
  if (current->array) skip += 1 + (current->value.u32 / sizeof(Node));
  
  return sweep(freelist, start+skip);
}

