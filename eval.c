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

Node * eval_and_chain(Node * args, Node * env)
{
  if (args == NIL) return NIL;
  // Here we make the exception to not automatically eval a block argument
  Node * result = args->special ? copy(args, 0) : eval(args, env);
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

    // TODO this has gone a bit ugly with 'special' added
    if (eval_args) var->next = index(args_as_list ? new_node(TYPE_NODE, index(eval_and_chain(args, caller_env))) : (args->special ? copy(args, 0) : eval(args, caller_env)));
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
  if(func == NIL) return pointer_to(NIL);

  // TODO this line is required to do the double evaluation
  // required to get the lambda-calculus style boolean values
  // working: ((< 1 2) 'j 'n) => (true 'j 'n) => 'j
  // Surely by now we can come up with something better?
  // E.g. implement TYPE_BOOL, seeing as in Scheme nil != #f anyway?
  if (func->type == TYPE_NODE) func = eval(pointer(func->value.u32), env);

  Node * args = pointer(funcexpr->next);

  switch(func->type)
  {
    case TYPE_INT:
      return run_integer(env, func, args);
      break;
    case TYPE_FUNC:
      return run_lambda(env, func, args, true);
      break;
    case TYPE_PRIMITIVE:
      return run_primitive(env, func, args);
      break;
    default:
      printf("Runtime error: can't execute type '%s'.\n", types[func->type]);
      return funcexpr;
      break;
  }
}

// Evaluate a single node, ignoring that it may be inside a list -
// but always returning the result as an element
Node * eval(Node * expr, Node * env)
{
  if (expr == NULL || expr == NIL) return expr;
  switch(expr->type)
  {
    case TYPE_ARG:
      return element(pointer(lookup(env, pointer(expr->value.u32))->next)); // TYPE_ARG points into the template env; lookup the corresponding var in the execution env
      break;
    case TYPE_VAR:
      return element(&memory[ memory[expr->value.u32].next ]); // TYPE_VAR is directly accessible, but skip the name and get the value part
      break;
    case TYPE_NODE:
      return apply(pointer(expr->value.u32), env);
      break;
    default:
      // TYPE_INT, TYPE_STRING, TYPE_ID (raw ID, not var), TYPE_NODE (raw data, not expr or block)
      return element(expr);
  }
}
