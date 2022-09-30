#include <string.h>

#include "node.h"
#include "memory.h"

#include "primitive.h"

// For cases with literal values, we could invent shorthand bytecode:
// push int val +1

Node * plus(Node * args, Node ** env)
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

Node * minus(Node * args, Node ** env)
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

Node * times(Node * args, Node ** env)
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

Node * div(Node * args, Node ** env)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  result->value.i32 = result->value.i32 / args->value.i32;
  return result;
}

Node * remain(Node * args, Node ** env)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = &memory[args->next];
  result->value.i32 = result->value.i32 % args->value.i32;
  return result;
}

// As per the rules for (no) recursiveness,
// We can't simply put 'env' in 'env' (and expect it to print);
// so instead supply a simple primitive.
Node * env(Node * args, Node ** env)
{
  return *env;
}
#define NUM_PRIMITIVES 6

char * primitives[NUM_PRIMITIVES] =
{
  "+", "-", "*", "/", "%", "env"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  plus, minus, times, div, remain, env
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  return -1;
}

