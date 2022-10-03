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

Node * lookup(Node * env, Node * name)
{
  if(env == NULL || env == NIL)
  {
    // This should be normally caught as a compile time error
    printf("Runtime error: cannot find variable '%s'\n", strval(name));
    return NIL; // by means of recovery??
  }

  if(strcmp(strval(name), strval(&memory[env->value.u32])) == 0) return &memory[env->value.u32];
  return lookup(&memory[env->next], name);
}


Node * set_variable(Node ** env, Node * expr)
{
  Node * var = &memory[expr->value.u32]; // no longer: lookup(*env, expr);
  Node * val = eval(&memory[expr->next], env);
  val->element = true;
  var->next = val - memory;
  return val;
}

// Returns the changed environment
Node * def_arg(Node * env, Node * name)
{
  name = copy(name, 0);
  name->element = false;
  name->next = 0;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, name - memory);
  newval->element = false;
  newval->next = env - memory;
  env = newval;
  return env;
}

Node * enclose(Node * env, Node * lambda)
{
  // Input: ((arglist) body-expr)
  // Result: ((env) (args) (transformed-body)
  // May be: ((env) (n-args) body))

  // Define lambda args as env variables:
  // By chaining them in at front, and the env is
  // naturally extended into the lambda's env.

  Node * argnames;
  if(lambda->type == TYPE_NODE) // usual case: (lambda (x) ...)
    argnames = &memory[lambda->value.u32];
  else
    argnames = element(lambda); // special case: (lambda x ...)

  // Define the variable slots.
  while (argnames != NIL)
  {
    env = def_arg(env, argnames);
    argnames = &memory[argnames->next];
  }
  
  // And transform expression
  Node * arglist = copy(lambda, 0);
  Node * body = transform_elem(&memory[lambda->next], &env);
  body->element = false;
  arglist->next = body - memory;

  // We define our closure as a list: (env args body)
  // But that is actually an implementation detail;
  // so that's why we return the result (a func) as a new type.
  // TODO in theory we don't even need to keep env, since we have pre-wired 'body'!
  Node * closure = new_node(TYPE_NODE, env - memory);
  closure->element=false;
  closure->next = arglist - memory;

  // We should return a (single) element
  return new_node(TYPE_FUNC, closure - memory);
}

Node * eval_and_chain(Node * args, Node ** env)
{
  if (args == NIL) return NIL;
  Node * result = eval(args, env);
  result->element = false;
  result->next = eval_and_chain(&memory[args->next], env) - memory;
  return result;
}

Node * run_lambda(Node ** env, Node * expr, Node * args)
{
  Node * lambda = &memory[expr->value.u32];

  Node * lambda_env = &memory[lambda->value.u32];
  
  bool args_as_list = true; // special case: (lambda x ...)
  Node * argnames = element(&memory[lambda->next]);
  if (argnames->type == TYPE_NODE)
  {
    args_as_list = false; // usual case: (lambda (x) ...)
    argnames = &memory[argnames->value.u32];
  }

  Node * body = &memory[ memory[lambda->next].next ];
  
  while (argnames != NULL && argnames != memory) // aka NIL
  {
    // In theory the args should be in a predictable position in the env,
    // but here we just do the lookup.
    if (argnames->element) args_as_list = true; // other special case: (lambda (x y . z) ...)
    Node * var = lookup(lambda_env, argnames);
    var->next = (args_as_list ? new_node(TYPE_NODE, idx(eval_and_chain(args, env))) : eval(args, env)) - memory;
    argnames = &memory[argnames->next];
    args = &memory[args->next];
  }

  return eval(body, &lambda_env);
}

Node * run_primitive(Node ** env, Node * prim, Node * args)
{
  return jmptable[prim->value.u32](eval_and_chain(args, env), env);
}

Node * perform_if (Node ** env, Node *args)
{
  Node * result = eval(args, env);
  // If any of the below is not supplied by the user,
  // it simply results in a chain of NIL evaluations,
  // which is what we want anyway.
  Node * thenn = &memory[args->next];
  Node * elsse = &memory[thenn->next];
  if(result == NIL) return eval(elsse, env);
  else return eval(thenn, env);
}
//
// EVAL / APPLY
//

// Evaluate a list expression, that is, apply function to args.
Node * apply(Node * funcexpr, Node ** env)
{
    // Allow sub-expression at function name position
    Node * func = eval(funcexpr, env);
    Node * args = &memory[funcexpr->next];

    switch(func->type)
    {
      case TYPE_ID:
        if(strcmp("define", strval(func)) == 0) return set_variable(env, args);
        if (strcmp("define-syntax", strval(func)) == 0) return set_variable(env, args);
        if(strcmp("set!", strval(func)) == 0) return set_variable(env, args);
        if(strcmp("lambda", strval(func)) == 0) return enclose((*env), args);
        if(strcmp("quote", strval(func)) == 0) return element(args);
        if(strcmp("if", strval(func)) == 0) return perform_if(env, args);
        // else
        printf("Runtime error: could not execute id '%s'.\n", strval(func));
        return funcexpr;
        break;
      case TYPE_FUNC:
        return run_lambda(env, func, args);
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
Node * eval(Node * expr, Node ** env)
{
  if (expr == NULL || expr == NIL) return expr;
  if(expr->type == TYPE_ID) return expr; // Since having variables transformed into TYPE_VAR below, any leftover IDs should be preserved
  if(expr->type == TYPE_VAR) return element(&memory[ memory[expr->value.u32].next ]);
  if(expr->type == TYPE_NODE) return apply( &memory[expr->value.u32], env);

  return element(expr);
}


