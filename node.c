#include <stdbool.h>

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

Node * chain(Type type, uint32_t value, Node * cdr)
{
  Node * result = new_node(type, value);
  result->element = false;
  result->next = index(cdr);
}
