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
  Node * argnames = &memory[lambda->value.u32];

  // Define that variables.
  // This is one point to change to support complicated
  // argument pattern matching (deconstruction) like &rest.
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

Node * run_lambda(Node ** env, Node * expr, Node * args)
{
  Node * lambda = &memory[expr->value.u32];

  Node * lambda_env = &memory[lambda->value.u32];
  Node * argnames =   &memory[ memory[lambda->next].value.u32 ];
  Node * body =   &memory[ memory[lambda->next].next ];
  
  while (argnames != NULL && argnames != memory) // aka NIL
  {
    // In theory the args should be in a predictable position in the env,
    // but here we just do the lookup.
    // This is the other point to change for &rest support, etc.
    Node * var = lookup(lambda_env, argnames);
    var->next = eval(args, env) - memory;
    argnames = &memory[argnames->next];
    args = &memory[args->next];
  }

  return eval(body, &lambda_env);
}

Node * eval_and_chain(Node * args, Node ** env)
{
  if (args == NIL) return NIL;
  Node * result = eval(args, env);
  result->element = false;
  result->next = eval_and_chain(&memory[args->next], env) - memory;
  return result;
}

Node * run_primitive(Node ** env, Node * prim, Node * args)
{
  return jmptable[prim->value.u32](eval_and_chain(args, env), env);
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
        if(strcmp("set!", strval(func)) == 0) return set_variable(env, args);
        if(strcmp("lambda", strval(func)) == 0) return enclose((*env), args);
        // else: var or special did not resolve. Maybe should return empty list.
        // Return original expression instead so that you have something to look at in REPL.
        return funcexpr;
        break;
      case TYPE_FUNC:
        return run_lambda(env, func, args);
        break;
      case TYPE_PRIMITIVE:
        return run_primitive(env, func, args);
        break;
      default:
        // The normal way to get here is because 'quote' preserved our list from transformation.
        // (Then afterwards 'quote' itself was removed from the data stream.)
        // In this case eval(funcexpr) should also not have done anything.
        // Nevertheless it is a bit cheeky that 'quote' actually causes these attempts.
        return funcexpr; // not usually reached
        break;
    }
}

// Evaluate a single node, ignoring that it may be inside a list -
// but always returning the result as an element
Node * eval(Node * expr, Node ** env)
{
  if (expr == NULL || expr == NIL) return expr;
  // if(expr->type == TYPE_ID) return lookup_value((*env), element(expr)); // should be deprecated by:
  if(expr->type == TYPE_VAR) return element(&memory[ memory[expr->value.u32].next ]);
  if(expr->type == TYPE_NODE) return apply( &memory[expr->value.u32], env);

  return element(expr);
}


