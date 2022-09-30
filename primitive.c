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

Node * eq (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return NIL;
  Node * rhs = &memory[lhs->next];
  if (lhs->type != rhs->type) return NIL;
  if (lhs->value.u32 != rhs->value.u32) return NIL;
  return NIL+1; // aka 'true'
}

Node * lt (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return NIL;
  Node * rhs = &memory[lhs->next];
  if (lhs->type != rhs->type) return NIL;
  return (lhs->value.i32 < rhs->value.i32) ? NIL+1 : NIL;
}

Node * gt (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return NIL;
  Node * rhs = &memory[lhs->next];
  if (lhs->type != rhs->type) return NIL;
  return (lhs->value.i32 > rhs->value.i32) ? NIL+1 : NIL;
}

// As per the rules for (no) recursiveness,
// We can't simply put 'env' in 'env' (and expect it to print);
// so instead supply a simple primitive.
Node * env(Node * args, Node ** env)
{
  return *env;
}
#define NUM_PRIMITIVES 9

char * primitives[NUM_PRIMITIVES] =
{
  "+", "-", "*", "/", "%", "=", "<", ">", "env"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  plus, minus, times, div, remain, eq, lt, gt, env
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  return -1;
}

