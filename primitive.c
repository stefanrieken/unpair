#include <string.h>

#include "node.h"
#include "memory.h"

#include "primitive.h"

// For cases with literal values, we could invent shorthand bytecode:
// push int val +1

Node * plus(Node * args, Node ** anv)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  while (args != NIL)
  {
    result->value.i32 += args->value.i32;
    args = &memory[args->next];
  }
  return result;
}

Node * minus(Node * args, Node ** anv)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  while (args != NIL)
  {
    result->value.i32 -= args->value.i32;
    args = &memory[args->next];
  }
  return result;
}

Node * times(Node * args, Node ** anv)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  while (args != NIL)
  {
    result->value.i32 *= args->value.i32;
    args = &memory[args->next];
  }
  return result;
}

Node * div(Node * args, Node ** anv)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  result->value.i32 = result->value.i32 / args->value.i32;
  return result;
}

Node * remain(Node * args, Node ** anv)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  result->value.i32 = result->value.i32 % args->value.i32;
  return result;
}

#define NUM_PRIMITIVES 5

char * primitives[NUM_PRIMITIVES] =
{
  "+", "-", "*", "/", "%"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  plus, minus, times, div, remain
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  return -1;
}

