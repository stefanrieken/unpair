#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "node.h"
#include "memory.h"
#include "eval.h"


//
// SPECIAL FUNCTIONS
//
Node * eval(Node * expr, Node ** env);

/**
 * n_recurse:
 * - 0 = none
 * - n = n times
 * - -n = indefinitely (or until int wraps around...)
 */
Node * copy(Node * node, int n_recurse)
{
  if (node == NULL) return NULL;
  Node * result = new_node(node->type, node->value.u32);
  // Allocate any overflow nodes
  int num_nodes = 0;
  if (node->array) num_nodes = (node->value.u32 / 8) + 1;
  for (int i=0; i< num_nodes; i++)
    allocate_node();

  memcpy(result, node, sizeof(Node) * (num_nodes+1));

  if (n_recurse > 0 && node->next != 0)
    result->next = copy(&memory[node->next], n_recurse-1) - memory;

  return result;
}

Node * element(Node * node)
{
  if (!node->element)
  {
    node = copy(node, 0);
    node->element = true;
    node->next = 0;
  }
  return node;
}

/**
 * expr: e.g. the arguments to 'define' (simplest form), so (label, value)
 * Returns new node & makes new env pointer.
 */
Node * def_variable(Node ** env, Node * expr)
{
  if(expr == NULL) return expr;
  Node * val = eval(&memory[expr->next], env);
  Node * var = copy(expr, 0);
  var->next = val - memory;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, var - memory);
  newval->element = false; // TODO is this still required after eval?
  newval->next = (*env) - memory;
  (*env) = newval;
  return val;
}

/**
 * As above, but tailored to setting lambda arguments.
 * Returns new env.
 */
Node * add_arg(Node * env, Node * name, Node * value)
{
  // Setup into a pair just like with 'def'
  // Copy this so as not to modify the expression.
  name = copy(name, 0);
  name->next = element(value) - memory;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, name - memory);
  newval->element = false;
  newval->next = env - memory;
  env = newval;
  return env;
}

Node * lookup(Node * env, Node * name)
{
  if(env == NULL || env == memory) return name; // return unresolved label

  if(strcmp(strval(name), strval(&memory[env->value.u32])) == 0) return &memory[env->value.u32];
  return lookup(&memory[env->next], name);
}

Node * lookup_value(Node * env, Node * name)
{
  Node * var = lookup(env, name);
  if (var->next == 0) return var; // lookup returns 'name' if not found; return that.
  return &memory[var->next];
}

Node * set_variable(Node ** env, Node * expr)
{
  Node * var = lookup(*env, expr);
  Node * next = eval(&memory[expr->next], env);
  next->element = true;
  var->next = next - memory;
  return next;
}

Node * enclose(Node * env, Node * lambda)
{
  // Result: ((env) (args) (body))
  Node * closure = new_node(TYPE_FUNC, env - memory);
  closure->element=false;
  closure->next = lambda - memory;
  return closure;
}

Node * run_lambda(Node ** env, Node * lambda, Node * args)
{
  Node * lambda_env = &memory[lambda->value.u32];
  Node * argnames =   &memory[ memory[lambda->next].value.u32 ];
  Node * body =   &memory[ memory[lambda->next].next ];
  
  while (argnames != NULL && argnames != memory) // aka NIL
  {
    lambda_env = add_arg(lambda_env, argnames, eval(args, env));
    argnames = &memory[argnames->next];
    args = &memory[args->next]; // TODO should eval these (NOT in lambda_env!)
  }

  return eval(body, &lambda_env);
}
//
// EVAL / APPLY
//


// Evaluate a list expression, that is, apply function to args.
Node * apply(Node * funcexpr, Node * args, Node ** env)
{
    // Allow sub-expression at function name position
    Node * func = eval(funcexpr, env);

    if (func->type == TYPE_ID)
    {
      if(strcmp("define", strval(func)) == 0) return def_variable(env, args);
      if(strcmp("set!", strval(func)) == 0) return set_variable(env, args);
      if(strcmp("lambda", strval(func)) == 0) return enclose((*env), args);
      // else: var or special did not resolve. Maybe should return empty list.
      // Return original expression instead so that you have something to look at in REPL.
      return funcexpr;
    }
    else if (func->type == TYPE_FUNC) return run_lambda(env, func, args);
    
    return funcexpr; // not usually reached
}

// Evaluate a single node, ignoring that it may be inside a list, so that:
// value => value as element(!)
// id => lookup(id)
// (..) => apply (..)
Node * eval(Node * expr, Node ** env)
{
  if (expr == NULL || expr == memory) return expr;
  if(expr->type == TYPE_ID) return lookup_value((*env), element(expr));
  if(expr->type == TYPE_NODE) return apply( &memory[expr->value.u32], &memory[ memory[expr->value.u32].next ], env);

  return element(expr);
}


