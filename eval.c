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
  return chain(TYPE_NODE, index(name), env);
}

Node * enclose(Node * env, Node * lambda)
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
  Node * body = transform_elem(pointer(lambda->next), &template_env, env);
  body->element = false;

  Node * closure = chain(TYPE_NODE, index(template_env),
                   chain(TYPE_NODE, index(env), // TODO in theory we don't even need to keep env, since we have pre-wired 'body'!
                   chain(TYPE_NODE, index(argnames), // really only needed to know where remainder args go at runtime
                   body)));

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

Node * instantiate_template(Node * template_env, Node * closure_env)
{
  if (template_env == NIL) return closure_env;

  Node * instance = copy(template_env, -1); // recursive copy
  Node * iter = instance;

  while(iter->next != 0)
  {
    iter->value.u32 = index(copy(pointer(iter->value.u32), -1)); // even more recursive copying
    iter = pointer(iter->next);
  }
  iter->value.u32 = index(copy(pointer(iter->value.u32), -1)); // even more recursive copying

  // Chain instance env to overall closure env
  iter->next = index(closure_env);
//print(instance);
//print(closure_env);

  return instance;
}

// lambda = ((env_template) (existing_env) (arglist) (body))
Node * run_lambda(Node * caller_env, Node * expr, Node * args, bool eval_args)
{
  Node * lambda = pointer(expr->value.u32);
  Node * env_node = pointer(lambda->next);

  Node * lambda_env = instantiate_template(pointer(lambda->value.u32), pointer(env_node->value.u32));
  //print(lambda_env);


  // Chain instance env to overall closure env
/*  Node * tail = lambda_env;
  while(tail->next != 0) tail = pointer(tail->next);
  tail->next = env_node->value.u32;*/

  bool args_as_list = true; // special case: (lambda x ...)
  Node * argnames = element(pointer(env_node->next));
  if (argnames->type == TYPE_NODE)
  {
    args_as_list = false; // usual case: (lambda (x) ...)
    argnames = pointer(argnames->value.u32);
  }

  Node * body = &memory[ memory[env_node->next].next ];

  while (argnames != NULL && argnames != NIL)
  {
    // Doing a lookup per arg is somewhat dumb, but the order
    // of the args within the lambda_env can be a bit messy,
    // e.g. for (lambda (x y) ((define z) ...)) you actually get [z y x ...]
    if (argnames->element) args_as_list = true; // other special case: (lambda (x y . z) ...)
    Node * var = lookup(lambda_env, argnames);

    if (eval_args) var->next = index(args_as_list ? new_node(TYPE_NODE, index(eval_and_chain(args, caller_env))) : eval(args, caller_env));
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
  if(expr->type == TYPE_ARG) return element(pointer(lookup(env, pointer(expr->value.u32))->next)); // TYPE_ARG points into the template env; lookup the corresponding var in the execution env
  if(expr->type == TYPE_VAR) return element(&memory[ memory[expr->value.u32].next ]); // TYPE_VAR is directly accessible, but skip the name and get the value part
  if(expr->type == TYPE_NODE) return apply( pointer(expr->value.u32), env);

  return element(expr);
}
