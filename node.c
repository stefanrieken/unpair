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

int mem_usage(Node * list)
{
  if (list == NIL) return 0;
  int len = 1;
  return len + mem_usage(&memory[list->next]);
}
