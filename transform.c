/**
 * Transform parsed form to a more pre-digested execution format, so that at
 * eval-time we do not need to repeat the same actions for the same outcome.
 * Such transformations may include:
 *
 * 1. Variable references are resolved to their value slots (DONE).
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

#include "node.h"
#include "eval.h"
#include "print.h"
#include "memory.h"

#define NIL (&memory[0])

Node * transform_expr(Node * expr, Node ** env);
Node * transform_elem(Node * elem, Node ** env);

/**
 * Define the variable, transforming but not eval'ing the variable expression.
 * expr: e.g. the arguments to 'define' (simplest form), so (label, value)
 * Returns new node & makes new env pointer.
 */
Node * define_variable(Node ** env, Node * expr)
{
  if (expr->next != 0)
  {
    Node * expr2 =copy(expr, 1);
    Node * name = &memory[expr2->next];
    name->next = transform_elem(&memory[ memory[expr->next].next ], env) - memory;

    // Chain into to environment (at front)
    Node * newval = new_node(TYPE_NODE, name - memory);
    newval->element = false; // TODO is this still required after eval?
    newval->next = (*env) - memory;
    (*env) = newval;
    return expr2;
  }
  // else
  return expr;
}

/**
 * Lookup a variable; return a TYPE_VAR reference, or the original value if not found.
 * (NOTE: the latter only for as long as we allow unresolved labels as values.)
  */
Node * dereference(Node * env, Node * name)
{
  if(env == NULL || env == memory) return name; // return unresolved label

  if(strcmp(strval(name), strval(&memory[env->value.u32])) == 0) return new_node(TYPE_VAR, env->value.u32);

  return dereference(&memory[env->next], name);
}


Node * transform_elem(Node * elem, Node ** env)
{
  if (elem->type == TYPE_ID)
  {
    //print_node(lookup_value((*env), elem), false);
    //return lookup_value((*env), elem);
    return dereference (*env, elem);
  }
  else if (elem->type == TYPE_NODE)
  {
    return new_node(TYPE_NODE, transform_expr(&memory[elem->value.u32], env) - memory);
  }
  // else
  return elem;
}

Node * transform_elements(Node * els, Node ** env)
{
  if (els == NIL) return NIL;
  Node * result = transform_elem(els, env);
  result->element = false;
  result->next = transform_elements(&memory[els->next], env) - memory;
  return result;
}

Node * transform_expr(Node * expr, Node ** env)
{
  if (expr->type == TYPE_ID)
  {
    if (strcmp("define", strval(expr)) == 0) return define_variable(env, expr);
    else if(strcmp("lambda", strval(expr)) == 0) return expr; // should likely transform body here; but first just prevent args from automatically being transformed as well
  }

  // default: transform all elements in expression
  return transform_elements(expr, env);
}
