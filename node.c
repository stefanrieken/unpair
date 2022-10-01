#include "node.h"
#include "memory.h"

char * types[] = {
  "int",
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
  if (list->array) len += 1 + (list->value.u32 / sizeof(Node));

  return len + mem_usage(&memory[list->next]);
}
