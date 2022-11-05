#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "node.h"
#include "memory.h"
#include "eval.h"
#include "primitive.h"
#include "print.h"
// ...because we didn't do it before:
#include "transform.h"

Node * set_variable(Node * env, Node * expr)
{
  Node * var = pointer(expr->value.u32); // no longer: lookup(*env, expr);
  Node * val = eval(pointer(expr->next), env);
  val->element = true;
  var->next = index(val);
  return val;
}

// Returns the changed environment
Node * def_arg(Node * env, Node * name)
{
  name = copy(name, 0);
  name->element = false;
  name->next = 0;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, index(name));
  newval->element = false;
  newval->next = index(env);
  env = newval;
  return env;
}

/**
 * Make arglist as array, so that it may
 * be instantiated by a single 'copy' call,
 * after which variables can (almost) directly refer to their index.
 */

Node * arglist_as_array(Node * arglist)
{
  int len = length(arglist);
  Node * result = new_array_node(TYPE_NODE, len << 3);

  Node * arg = &nodearray(result)[0]; // may be out-of-bounds; then while is not entered
  while (arglist != NIL)
  {
    arg->mark = false;
    arg->element = false; // ?
    arg->type = arglist->type;
    arg->array = false; if (arglist->array) printf("Error: can't in-line array\n");
    arg->value = arglist->value;
    arg->next = arglist->next == 0 ? 0 : index(arg+1);
    
    arg += 1;
    arglist = &memory[arglist->next];
  }
  return result;
}

Node * enclose(Node * env, Node * lambda)
{
  // Input: ((arglist) body-expr)
  // Result: ((env) (args) (transformed-body)

  // Define lambda args as env variables:
  // By chaining them in at front, and the env is
  // naturally extended into the lambda's env.

  Node * argnames;
  if(lambda->type == TYPE_NODE) // usual case: (lambda (x) ...)
    argnames = pointer(lambda->value.u32);
  else
    argnames = element(lambda); // special case: (lambda x ...)

  // Define the variable slots.
  while (argnames != NIL)
  {
    env = def_arg(env, argnames);
    argnames = pointer(argnames->next);
  }
  
  // And transform expression
  Node * arglist = copy(lambda, 0);
  Node * body = transform_elem(pointer(lambda->next), &env);
  body->element = false;
  arglist->next = index(body);

  // We define our closure as a list: (env args body)
  // But that is actually an implementation detail;
  // so that's why we return the result (a func) as a new type.
  // TODO in theory we don't even need to keep env, since we have pre-wired 'body'!
  Node * closure = new_node(TYPE_NODE, index(env));
  closure->element=false;
  closure->next = index(arglist);

  // We should return a (single) element
  return new_node(TYPE_FUNC, index(closure));
}

Node * eval_and_chain(Node * args, Node * env)
{
  if (args == NIL) return NIL;
  Node * result = eval(args, env);
  result->element = false;
  result->next = index(eval_and_chain(&memory[args->next], env));
  return result;
}

Node * run_lambda(Node * env, Node * expr, Node * args, bool eval_args)
{
  Node * lambda = pointer(expr->value.u32);

  Node * lambda_env = pointer(lambda->value.u32);
  
  bool args_as_list = true; // special case: (lambda x ...)
  Node * argnames = element(pointer(lambda->next));
  if (argnames->type == TYPE_NODE)
  {
    args_as_list = false; // usual case: (lambda (x) ...)
    argnames = pointer(argnames->value.u32);
  }

  Node * body = &memory[ memory[lambda->next].next ];
  
  while (argnames != NULL && argnames != memory) // aka NIL
  {
    // In theory the args should be in a predictable position in the env,
    // but here we just do the lookup.
    if (argnames->element) args_as_list = true; // other special case: (lambda (x y . z) ...)
    Node * var = lookup(lambda_env, argnames);

    if (eval_args) var->next = index(args_as_list ? new_node(TYPE_NODE, index(eval_and_chain(args, env))) : eval(args, env));
    else var->next = index(args_as_list ? new_node(TYPE_NODE, index(args)) : element(args));
    argnames = pointer(argnames->next);
    args = pointer(args->next);
  }

  return eval(body, lambda_env);
}

Node * run_primitive(Node * env, Node * prim, Node * args)
{
  // We do not presently add to the env from within primitives,
  // nor is this a particularly good idea - so then perhaps
  // we should not suggest it by passing the env as a double
  // pointer, as we still do here.
  return jmptable[prim->value.u32](eval_and_chain(args, env), &env);
}

Node * perform_if (Node * env, Node *args)
{
  Node * result = eval(args, env);
  // If any of the below is not supplied by the user,
  // it simply results in a chain of NIL evaluations,
  // which is what we want anyway.
  Node * thenn = pointer(args->next);
  Node * elsse = pointer(thenn->next);
  if(result == NIL) return eval(elsse, env);
  else return eval(thenn, env);
}

Node * run_integer(Node * env, Node * func, Node * args)
{
  // Here's a(nother) novel idea:
  // (2 '("foo" "bar" "baz")) == "bar"
  // May or may not be better than implementing ('("foo" "bar" "baz") 2) instead...
  // Also, count from zero?

  args = eval_and_chain(args, env);
  Node * list = pointer(args->value.u32);
  int32_t n = func->value.i32;
  for (int i=1; i<n; i++)
    list = pointer(list->next);
  return element(list);
}

//
// EVAL / APPLY
//

// Evaluate a list expression, that is, apply function to args.
Node * apply(Node * funcexpr, Node * env)
{
  // Allow sub-expression at function name position
  Node * func = eval(funcexpr, env);
  if(func == NIL) return func;

  Node * args = pointer(funcexpr->next);

  char * chars;

  switch(func->type)
  {
    case TYPE_INT:
      return run_integer(env, func, args);
      break;
    case TYPE_ID:
      chars = strval(pointer(func->value.u32));
      if(strcmp("define", chars) == 0) return set_variable(env, args);
      if (strcmp("define-syntax", chars) == 0) return set_variable(env, args);
      if(strcmp("set!", chars) == 0) return set_variable(env, args);
      if(strcmp("lambda", chars) == 0) return enclose(env, args);
      if(strcmp("quote", chars) == 0) return element(args);
      if(strcmp("if", chars) == 0) return perform_if(env, args);
      // else
      printf("Runtime error: could not execute id '%s'.\n", strval(func));
      return funcexpr;
      break;
    case TYPE_FUNC:
      return run_lambda(env, func, args, true);
      break;
    case TYPE_PRIMITIVE:
      return run_primitive(env, func, args);
      break;
    default:
      printf("Runtime error: can't execute type '%s'.\n", types[func->type]);
      return funcexpr; // not usually reached
      break;
  }
}

// Evaluate a single node, ignoring that it may be inside a list -
// but always returning the result as an element
Node * eval(Node * expr, Node * env)
{
  if (expr == NULL || expr == NIL) return expr;
  if(expr->type == TYPE_ID) return expr; // Since having variables transformed into TYPE_VAR below, any leftover IDs should be preserved
  if(expr->type == TYPE_VAR) return element(&memory[ memory[expr->value.u32].next ]); // Get the value part of the VAR
  if(expr->type == TYPE_NODE) return apply( pointer(expr->value.u32), env);

  return element(expr);
}


