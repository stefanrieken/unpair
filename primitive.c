#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "node.h"
#include "memory.h"

#include "primitive.h"
#include "gc.h"
#include "transform.h"
#include "eval.h"
#include "print.h"

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
    args = pointer(args->next);
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
    args = pointer(args->next);
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
  if (lhs == NIL || lhs->next == 0) return pointer_to(NIL);
  Node * rhs = pointer(lhs->next);
  if (lhs->type != rhs->type) return pointer_to(NIL);
  if (lhs->value.u32 != rhs->value.u32) return pointer_to(NIL);
  return pointer_to(NIL+1); // aka 'true'
}

Node * lt (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return pointer_to(NIL);
  Node * rhs = &memory[lhs->next];
  if (lhs->type != rhs->type) return pointer_to(NIL);
  return (lhs->value.i32 < rhs->value.i32) ? pointer_to(NIL+1) : (NIL);
}

Node * gt (Node * lhs, Node ** env)
{
  if (lhs == NIL || lhs->next == 0) return pointer_to(NIL);
  Node * rhs = pointer(lhs->next);
  if (lhs->type != rhs->type) return pointer_to(NIL);
  return (lhs->value.i32 > rhs->value.i32) ? pointer_to(NIL+1) : pointer_to(NIL);
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
  if (list == NIL) return pointer_to(NIL);

  Node * result = pointer(list->next);
  if(result->element && (result != NIL)) return result; // deconstruct Pair
  else return pointer_to(pointer(list->next));
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
  // The exception for value at index 1 ('true')
  // is because it is technically a singleton
  // implemented as a pointer to a single node.
  // Maybe introduce a special type for this?
  if (cdr->type == TYPE_NODE && cdr->value.u32 != 1)
  {
    Node * cdrlist = pointer(cdr->value.u32);
    if (!cdrlist->element) // then it's a singleton pointer
    {
      car = copy(car, 0);
      cdr = pointer(cdr->value.u32);
      //cdr->element = false;
      car->next = index(cdr);
      return new_node(TYPE_NODE, index(car));
    }
  }
  // so, if not a node, or if a singleton:
  // Pretend to be a pair
  car = copy(car, 1);
  cdr = pointer(car->next);
  cdr->element = true;
  return new_node(TYPE_NODE, index(car));
}

// As per the rules for (no) recursiveness,
// We can't simply put 'env' in 'env' (and expect it to print);
// so instead supply a simple primitive.
Node * env(Node * args, Node ** env)
{
  return *env;
}

Node * is_element(Node * args, Node ** env)
{
  Node * val = pointer(args->value.u32);
  return val->element ? pointer_to(NIL+1) : pointer_to(NIL);
}

//
// SPECIAL FORM PRIMITIVES
//
Node * iff (Node *test, Node ** env)
{
  // If any of the below is not supplied by the user,
  // it simply results in a chain of NIL evaluations,
  // which is what we want anyway.
  Node * thenn = pointer(test->next);
  Node * elsse = pointer(thenn->next);
  if(test->value.u32 == 0) return eval(elsse, *env);
  else return eval(thenn, *env);
}

Node * setvar(Node * expr, Node ** env)
{

  Node * var = pointer(expr->value.u32);
  Node * val = element(pointer(expr->next)); //eval(pointer(expr->next), *env);
  var->next = index(val);
  return val;
}

// Returns the changed environment
static Node * def_arg(Node * env, Node * name)
{
  name = copy(name, 0);
  name->element = false;
  name->next = 0;

  // Chain into to environment (at front)
  return chain(TYPE_NODE, index(name), env);
}

Node * enclose(Node * lambda, Node ** env)
{
  // Input: ((arglist) body-expr)
  // Result: ((template_env) (existing_env) (arglist) (transformed-body)

  // Define lambda args as env variables:
  // By chaining them in at front, and the env is
  // naturally extended into the lambda's env.

  Node * argnames;
  if(lambda->type == TYPE_NODE) // usual case: (lambda (x) ...)
    argnames = pointer(lambda->value.u32);
  else
    argnames = element(lambda); // special case: (lambda x ...)

  // We construct a template env to hold the args and per-instance variables
  // (= variables added by method-local 'define's). This template should be cloned
  // during run_lambda to give every instance its own set of local vars.
  // This as opposed to the surrounding env, which is fully constructed before
  // we are called, and so may be accessed by means of direct pointers.
  // By also making 'transform' aware of these difference, it can optimize
  // variable references for either case.
  Node * template_env = NIL;
  Node * argnames_iter = argnames;
  // Define the variable slots.
  while (argnames_iter != NIL)
  {
    template_env = def_arg(template_env, argnames_iter);
    argnames_iter = pointer(argnames_iter->next);
  }

  // And transform expression
  Node * body = transform_elem(pointer(lambda->next), &template_env, *env);
  body->element = false;

  Node * closure = chain(TYPE_NODE, index(template_env),
                   chain(TYPE_NODE, index(*env), // TODO in theory we don't even need to keep env, since we have pre-wired 'body'!
                   chain(TYPE_NODE, index(argnames), // really only needed to know where remainder args go at runtime
                   body)));

  // We should return a (single) element
  return new_node(TYPE_FUNC, index(closure));
}

Node * print_string(Node * expr, Node ** env)
{
  while (expr != NIL)
  {
    if (expr->type == TYPE_INT)
      printf("%d", expr->value.i32);
    else if (expr->type == TYPE_STRING || expr->type == TYPE_ID)
      printf("%s", strval(pointer(expr->value.u32)));

    expr = pointer(expr->next);
  }

  return pointer_to(NIL);
}

Node * eval_cb(Node * expr, Node ** env)
{
  return eval(transform(expr, env, *env), *env);
}

Node * apply_cb(Node * expr, Node ** env)
{
  return eval(transform(expr, env, *env), *env);
}

#define NUM_PRIMITIVES 20

char * primitives[NUM_PRIMITIVES] =
{
  // Integer arithmetic primitives
  "+", "-", "*", "/", "%", "=", "<", ">",
  // Utility
  "print",
  // List primitives
  "car", "cdr", "cons",
  // Reflection primitives
  "eval", "env", "element?",
  // Special form primitives:
  "lambda", "if", "define", "define-syntax", "set!"
};

PrimitiveCb jmptable[NUM_PRIMITIVES] =
{
  // Integer arithmetic primitives
  plus, minus, times, div, remain, eq, lt, gt,
  // Utility
  print_string,
  // List primitives
  car, cdr, cons,
  // Reflection primitives
  eval_cb, env, is_element,
  // Special form primitives - notice anything?
  enclose, iff, setvar, setvar, setvar
};

int find_primitive(char * name)
{
  for (int i=0; i< NUM_PRIMITIVES; i++) if (strcmp(primitives[i], name) == 0) return i;
  printf("not found: %s\n", name);
  return -1;
}
