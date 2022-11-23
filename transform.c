/**
 * Transform parsed form to a more pre-digested execution format, so that at
 * eval-time we do not need to repeat the same actions for the same outcome.
 * Such transformations may include:
 *
 * 1. Variable references are resolved to their value slots: [v] DONE
 *    N.b.: this is NOT the same as resolving to their values;
 *    to support changeable in-line values, we still need an indirection.
 *    This has the advantage that the eval-time TYPE_VAR data is different
 *    from other expressions requiring further evaluation.
\ *
 * 2. Sub-expressions that are to be executed are in-lined; "special" ones are not.
 *    This eleminates the difference between special functions and primitives at runtime.
 *    But it also requires markers for start / end of sub-expression or number of args.
 *
 * 3. Lambda calls are converted to functions with environments ("closed" into "closures").
 *
 * 4. Code is stored in a consecutive array
 *    This frees up the 'next' portion of the data nodes, which in turn may hold the
 *    marker(s) required by #2. (Beyond that, it is hard to save this space from being
 *    wasted in alignment; a more compressed byte code may eliminate it.)
 *
 * 5. Code is converted to execution order (postfix)
 *    This helps implementing our own execution stack; and from there performing things
 *    like tail call optimization.
 *
 * Examples (envisioned):
 *
 *    ((if (< y 10) (foo) (bar)))
 *
 *    push node (bar)  ;; 'push' is the defaut behaviour; 'node' is the value tag (node pointer)
 *    push node (foo)  ;; (foo) and (bar) are recognized as (special) bloock arguments to 'if' and are transformed but not yet executed
 *    push int  10     ;; 'cond' expression is in-lined
 *    push slot *y     ;; take the value from the variable slot and push that (so stack content differs from given value 'y' and tag 'slot')
 *    call if   3      ;; #args is either stored in 'next', indicating 'eval'; or first push 'if' then separate command 'eval 3'
 *
 *    (define x (lambda (y) 42)
 *
 *    push func   (env (y) 42) ;; enclosed lambda
 *    push label  x            ;; special form: label is not dereferenced
 *    call define 3            ;; as above
 *
 *    (apply '+ '(1 2))
 *
 *    push node (1 2)  ;; quote prevents both transformation and evaluation of value; actual call to 'quote' is no longer required.
 *    push label +     ;; quoted label is not dereferenced
 *    call apply 3     ;; it is up to the implementation of 'apply' to dereference '+' and transform '(1 2)'
 *
 *    (set! x 42)
 *
 *    Either:
 *    push int 42
 *    push slot *x
 *    call set! 3
 *
 *    Or:
 *    push int 42
 *    push label x
 *    call set! 3
 *
 *    Or:
 *    push int 42
 *    set  slot *x     ;; special command?
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "node.h"
#include "eval.h"
#include "print.h"
#include "memory.h"
#include "primitive.h"

#include "transform.h"
#include "eval.h"
// For unique_string and friends
#include "parse.h"

Node * transform_quote(Node * value)
{
  Node * result = copy(value, 0);
  result->element = true;
  result->special = true;
  return result;
}
/**
 * Transform the expressions for define and set! to their eval-time form.
 */
Node * transform_if(Node ** constructing_env, Node * existing_env, Node * expr)
{
  Node * iff =  new_node(TYPE_PRIMITIVE, find_primitive("if"));
  iff->element = false;

  Node * test = pointer(expr->next);
  Node * thenn = pointer(test->next);
  Node * elsse = pointer(thenn->next);

  test = transform_elem(test, constructing_env, existing_env);
  thenn = transform_elem(thenn, constructing_env, existing_env);
  thenn->special = true;
  elsse = transform_elem(elsse, constructing_env, existing_env);
  elsse->special = true;

  iff->next = index(test);
  test->next = index(thenn);
  thenn->next = index(elsse);

  return iff;
}

 // Lambda should be eval'ed at eval time; and its args should be regarded as plain data up to that point.
Node * transform_lambda(Node * expr)
{
   Node * lambda =  new_node(TYPE_PRIMITIVE, find_primitive("lambda"));
   lambda->element = false;
   Node * args = copy(pointer(expr->next), 1);
   args->special = true;
   Node * body = pointer(args->next);
   body->special = true;
   
   lambda->next = index(args);
   return lambda;
}
/**
 * Transform the expressions for define and set! to their eval-time form.
 */
Node * transform_set(Node ** constructing_env, Node * existing_env, Node * expr)
{
  if (expr->next != 0)
  {
    Node * name = pointer(expr->next);
    Node * value = pointer(name->next); // may be NIL

//    Node * expr2 = copy(expr, 0);
    Node * label = pointer(expr->value.u32);
    Node * expr2 =  new_node(TYPE_PRIMITIVE, find_primitive(strval(label)));
    expr2->element = false;
    Node * var = transform_elem(name, constructing_env, existing_env); // does the VAR lookup, or any other applicable shenanigans. NOTE: no true expressions are expected here
    var->element = false;
    var->special = true;

    Node * val = transform_elem(value, constructing_env, existing_env);

    expr2->next = index(var);
    var->next = index(val);
    return expr2;
  }
  // else
  printf ("Compile error: missing variable name in 'set' expression.\n");
  return NULL;
}

/**
 * Define the variable, transforming but not eval'ing the variable expression.
 * expr: e.g. the arguments to 'define' (simplest form), so (label, value)
 *
 * Returns the transformed form for further execution upon 'eval' time.
 */
Node * define_variable(Node ** def_env, Node * transform_env, Node * expr)
{
  if (expr->next != 0)
  {
    Node * name = pointer(expr->next);
    if (name->type == TYPE_NODE)
    {
      // We have the syntactic sugar version: (define (f x) ...)
      // (not quite) 'macrotransform' this to (define f (lambda (x) ...))
      Node * body = pointer(name->next);
      name = pointer(name->value.u32);

      Node * lambda = chain(TYPE_ID, index(unique_string(make_char_array_node("lambda"))), // "lambda"
                      chain(TYPE_NODE, name->next, // (x)
                      body)); // ...

      expr = chain(expr->type, expr->value.u32, // "define"
             chain(TYPE_ID, name->value.u32,  // "f"
             chain(TYPE_NODE, index(lambda), NIL))); // (lambda x ...)
             //print(expr);
    }
    // Chain into to environment (at front)
    (*def_env) = chain(TYPE_NODE, index(copy(name, 0)), *def_env);

    // Transform the run-time expression (borrow code for 'set')
    return transform_set(def_env, transform_env, expr);
  }
  // else
  printf ("Compile error: missing variable name in 'define' expression.\n");
  return NULL;
}

Node * as_primitive(Node * label)
{
  int num = find_primitive(strval(pointer(label->value.u32)));
  if (num < 0) return NIL;
  else return new_node(TYPE_PRIMITIVE, num);
}

extern Node * macros;

Node * macrotransform(Node * expr, Node * env)
{
  if (macros == NIL) return expr;

  Node * macro = find_macro(macros, expr);

  // Keep executing macros until final form is reached
  while (macro != NIL)
  {
    //print(macro);

    // Execute common lambda, but make sure it does
    // not evaluate its args, which are now code (fragments)
    // TODO: we presently only pass it existing_env,
    // and not the env presently under construction -
    // but shouldn't the macro be executed purely in the macros env?
    expr = pointer(run_lambda(env, macro, expr, false)->value.u32);
    macro = find_macro(macros, expr);
  }
  return expr;
}

Node * transform_elem(Node * elem, Node ** constructing_env, Node * existing_env)
{
  if (elem->type == TYPE_ID)
  {
    // Lookup the value location in memory by name,
    // returning a TYPE_VAR.
    Node * result = dereference(*constructing_env, elem, TYPE_ARG);
    if (result == NIL) result = dereference(existing_env, elem, TYPE_VAR);
    if (result == NIL) result = as_primitive(elem);
    if (result == NIL) printf("Compilation error: '%s' not found.\n", strval(&memory[elem->value.u32]));
    return result;
  }
  else if (elem->type == TYPE_NODE && elem->value.u32 != 0)
  {
    Node * result = transform_expr(&memory[elem->value.u32], constructing_env, existing_env);

    // Detect .->(quote x) => .-> (x)
    // To return it as x, not as (x).
    if (result->element)
    {
      result->element = elem->element;
      return result;
    }
    else
    {
      result = new_node(TYPE_NODE, index(result));
      result->element = elem->element;
      return result;
    }
  }
  // else
  return elem;
}

Node * transform_elements(Node * els, Node ** constructing_env, Node * existing_env)
{
  if (els == NIL) return NIL;
  Node * result = transform_elem(els, constructing_env, existing_env);
  if (result == NIL) return NIL;
  result->element = els->element;
  result->next = index(transform_elements(&memory[els->next], constructing_env, existing_env));
  return result;
}

Node * transform_expr(Node * expr, Node ** constructing_env, Node * existing_env)
{
  Node * expr2 = macrotransform(expr, existing_env);

  if (expr2 != expr)
  {
    //print(expr2); // I guess to properly print the node it should be wrapped into an element first
    expr = expr2;
  }

  if (expr->type == TYPE_ID)
  {
    char * chars = strval(pointer(expr->value.u32));
    if (strcmp("define", chars) == 0) return define_variable(constructing_env, existing_env, expr);
    if (strcmp("define-syntax", chars) == 0) return define_variable(&macros, existing_env, expr);
    if (strcmp("set!", chars) == 0) return transform_set(constructing_env, existing_env, expr);
    if (strcmp("lambda", chars) == 0) return transform_lambda(expr);
    if (strcmp("quote" , chars) == 0) return transform_quote(pointer(expr->next)); //element(pointer(expr->next)); // because after this step, raw labels and nodes are recognized as data
    if (strcmp("if", chars) == 0) return transform_if(constructing_env, existing_env, expr);
    // else - find primitive or user defined function
    return transform_elements(expr, constructing_env, existing_env);
  }
  // else
  return transform_elements(expr, constructing_env, existing_env);
}

Node * transform(Node * node, Node ** constructing_env, Node * existing_env)
{
    if (node->element) return transform_elem(node, constructing_env, existing_env);
    else return transform_expr(node, constructing_env, existing_env);
}
