#include "node.h"
#include "memory.h"

char * types[] = {
  "int",
  "char",
  "string",
  "id",
  "node",
  "func",
  "var",
  "primitive"
};

int length(Node * list)
{
  if (list == NIL) return 0;
  return 1 + length(&memory[list->next]);
}

int mem_usage(Node * list)
{
  if (list == NIL) return 0;
  return 1 + mem_usage(&memory[list->next]);
}
