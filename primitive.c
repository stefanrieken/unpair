#include <string.h>
#include <stdbool.h>

#include "node.h"
#include "memory.h"

#include "primitive.h"
#include "gc.h"

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

Node * car (Node * val, Node ** env)
{
  Node * list = &memory[val->value.u32];
  return element(list);
}

Node * cdr (Node * val, Node ** env)
{
  Node * list = &memory[val->value.u32];
  // don't repackage NIL result into a single pointer-with-type result
  if (list == NIL) return list;

  Node * result = &memory[list->next];
  if(result->element) return result; // deconstruct Pair
  else return new_node(TYPE_NODE, list->next);
}

// Make (copy) a node with car value of arg1,
// and cdr value of arg2.
// This requires a bit of translation in our case:
// (cons a b)    => (a . b) -> arg1.next = element(arg2)
// (cons a (b))  => (a b)   -> arg1.next = arg2.value
// (cons a ())   => (a)     -> arg1.next = arg2.value

Node * cons (Node * car, Node ** env)
{
  Node * cdr = &memory[car->next];
  if (cdr->type == TYPE_NODE)
  {
    // In-line the sublist
    car = copy(car, 0);
    cdr = copy(&memory[cdr->value.u32], 0);
    //cdr->element = false;
    car->next = cdr - memory;
  }
  else
  {
    // Pretend to be a pair
    car = copy(car, 1);
    cdr = &memory[car->next];
    cdr->element = true;
  }
  
  // Even a single cons cell returned
  // should be presented as an Element
  // with type tagging:
  return new_node(TYPE_NODE, idx(car));
}

// As per the rules for (no) recursiveness,
// We can't simply put 'env' in 'env' (and expect it to print);
// so instead supply a simple primitive.
Node * env(Node * args, Node ** env)
{
  return *env;
}

// Only call this in a top-level expression!
// (at least until we control our own stack)
Node * gc(Node * args, Node ** env)
{
  int marked = 0;
  // This is all we can reach from here... it should suffice
  marked += mark(NIL);
  marked += mark(NIL+1); // true
  // marked += mark(freelist);
  marked += mark(args);
  marked += mark (*env);

  freelist = sweep();
  int freesize=mem_usage(freelist);

  // for now just return garbage stats
  Node * stats = new_node(TYPE_INT, memsize);
  stats->element = false;
  Node * marked_node = new_node(TYPE_INT, marked);
  marked_node->element = false;
  stats->next = marked_node - memory;
  Node * swept_node = new_node(TYPE_INT, freesize);
  swept_node->element = false;
  marked_node->next = swept_node - memory;
  return stats;
}

#define NUM_PRIMITIVES 13

char * primitives[NUM_PRIMITIVES] =
{
  "+", "-", "*", "/", "%", "=", "<", ">", "car", "cdr", "cons", "env", "gc"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  plus, minus, times, div, remain, eq, lt, gt, car, cdr, cons, env, gc
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  return -1;
}

