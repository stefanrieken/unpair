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
  args = pointer(args->next);
  while (args != NIL)
  {
    result->value.i32 += args->value.i32;
    args = pointer(args->next);
  }
  return result;
}

Node * minus(Node * args, Node ** env)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = pointer(args->next);
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
  args = pointer(args->next);
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
  args = pointer(args->next);
  result->value.i32 = result->value.i32 / args->value.i32;
  return result;
}

Node * remain(Node * args, Node ** env)
{
  Node * result = new_node(TYPE_INT, args->value.i32);
  args = pointer(args->next);
  result->value.i32 = result->value.i32 % args->value.i32;
  return result;
}

Node * eq (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return NIL;
  Node * rhs = pointer(lhs->next);
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
  Node * rhs = pointer(lhs->next);
  if (lhs->type != rhs->type) return NIL;
  return (lhs->value.i32 > rhs->value.i32) ? NIL+1 : NIL;
}

Node * car (Node * val, Node ** env)
{
  Node * list = pointer(val->value.u32);
  return element(list);
}

Node * cdr (Node * val, Node ** env)
{
  Node * list = pointer(val->value.u32);
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
  Node * cdr = pointer(car->next);
  if (cdr->type == TYPE_NODE)
  {
    // In-line the sublist
    car = copy(car, 0);
    cdr = copy(pointer(cdr->value.u32), 0);
    //cdr->element = false;
    car->next = index(cdr);
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
  return new_node(TYPE_NODE, index(car));
}

// As per the rules for (no) recursiveness,
// We can't simply put 'env' in 'env' (and expect it to print);
// so instead supply a simple primitive.
Node * env(Node * args, Node ** env)
{
  return *env;
}

#define NUM_PRIMITIVES 12

char * primitives[NUM_PRIMITIVES] =
{
  "+", "-", "*", "/", "%", "=", "<", ">", "car", "cdr", "cons", "env"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  plus, minus, times, div, remain, eq, lt, gt, car, cdr, cons, env
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  return -1;
}

