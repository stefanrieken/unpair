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

/**
 * Transform the expressions for define and set! to their eval-time form.
 */
Node * transform_if(Node ** env, Node * expr)
{
  Node * iff = copy(expr, 0);
  iff->next = index(transform_elements(pointer(expr->next), env));
  return iff;
}

/**
 * Transform the expressions for define and set! to their eval-time form.
 */
Node * transform_set(Node ** def_env, Node ** transform_env, Node * expr)
{
  if (expr->next != 0)
  {
    Node * name = pointer(expr->next);
    Node * value = pointer(name->next); // may be NIL

    Node * expr2 =copy(expr, 0);
    Node * var = transform_elem(name, def_env); // does the VAR lookup, or any other applicable shenanigans
    var->element = false;
    Node * val = transform(value, transform_env);

    expr2->next = var-memory;
    var->next = val-memory;

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
 * Returns the transformed form fur further execution upon 'eval' time.
 */
Node * define_variable(Node ** def_env, Node ** transform_env, Node * expr)
{
  if (expr->next != 0)
  {
    // Make new env entry
    Node * name = copy(pointer(expr->next), 0);
    name->element = false;
    name->next = 0;

    // Chain into to environment (at front)
    Node * newval = new_node(TYPE_NODE, index(name));
    newval->element = false;
    newval->next = index(*def_env);
    (*def_env) = newval;

    // Change the run-time expression (borrow code for 'set')
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

Node * macrotransform(Node * expr, Node ** env)
{
  if (macros == NIL) return expr;

  Node * macro = find_macro(macros, expr);

  // Keep executing macros until final form is reached
  while (macro != NIL)
  {
    // Execute common lambda, but make sure it does
    // not evaluate its args, which are now code (fragments)
    expr = pointer(run_lambda(*env, macro, expr, false)->value.u32);
    macro = find_macro(macros, expr);
  }
  return expr;
}

Node * transform_elem(Node * elem, Node ** env)
{
  if (elem->type == TYPE_ID)
  {
    // Lookup the value location in memory by name,
    // returning a TYPE_VAR.
    // TODO this is where recursive calls go awry
    Node * result = dereference(*env, elem);
    if (result == NIL) result = as_primitive(elem);
    if (result == NIL) {print(*env); printf("Compilation error: '%s' not found.\n", strval(&memory[elem->value.u32])); }
    return result;
  }
  else if (elem->type == TYPE_NODE)
  {
    return new_node(TYPE_NODE, index(transform_expr(&memory[elem->value.u32], env)));
  }
  // else
  return elem;
}

Node * transform_elements(Node * els, Node ** env)
{
  if (els == NIL) return NIL;
  Node * result = transform_elem(els, env);
  if (result == NIL) return NIL;
  result->element = els->element;
  result->next = index(transform_elements(&memory[els->next], env));
  return result;
}

Node * transform_expr(Node * expr, Node ** env)
{
  Node * expr2 = macrotransform(expr, env);

  if (expr2 != expr)
  {
    //print(expr2); // I guess to properly print the node it should be wrapped into an element first
    expr = expr2;
  }

  if (expr->type == TYPE_ID)
  {
    // Still have to compare strings until labels are unique in memory
    char * chars = strval(pointer(expr->value.u32));
    if (strcmp("define", chars) == 0) return define_variable(env, env, expr);
    if (strcmp("define-syntax", chars) == 0) return define_variable(&macros, env, expr);
    if (strcmp("set!", chars) == 0) return transform_set(env, env, expr);
    if (strcmp("lambda", chars) == 0) return expr; // Lambda should be eval'ed at eval time; and its args should be regarded as plain data up to that point.
    if (strcmp("quote" , chars) == 0) return expr; // This one too.
    if (strcmp("if", chars) == 0) return transform_if(env, expr);
    // else - find primitive or user defined function
    return transform_elements(expr, env);
  }
  // else
  return transform_elements(expr, env);
}

Node * transform(Node * node, Node ** env)
{
    if (node->element) return transform_elem(node, env);
    else return transform_expr(node, env);  
}

