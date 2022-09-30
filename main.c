#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	
#include <unistd.h>

#include "node.h"
#include "memory.h"
#include "print.h"
#include "parse.h"
#include "eval.h"

#include "transform.h"

// for 1-char args only!
Node * make_arg(char * arg)
{
  Node * argnode = new_node(TYPE_ID, 2);
  char * value = (char *) allocate_node();
  strcpy (value, arg);
  argnode->array = true;
  return argnode;
}

Node * def_arg(Node * env, Node * name);
Node * lookup(Node * env, Node * name);

// An attempt at lambda-calculus style boolean values.
// They are at memory locations 0 (false, empty list, NIL) and 1 (true)
// TODO: since these are normal, (non-lazy / special) functions,
// both the 'true' and 'false' arguments are evaluated, so presently
// this cannot replace a complete 'if' implementation.
void make_boolean(Node * slot)
{
  Node * env = NIL;
 
  Node * a = make_arg("a");
  Node * b = make_arg("b");

  env = def_arg(env, a);
  env = def_arg(env, b);

  a->element=false;
  b->element=false;
  a->next = b - memory;
  b->next = 0;

  Node * arglist = new_node(TYPE_NODE, a - memory);
  arglist->element=false;

  Node * closure = new_node(TYPE_NODE, env - memory);
  closure->element = false;
  closure->next = arglist - memory;

  Node * which = (slot == NIL) ? b : a;
  Node * outcome = new_node(TYPE_VAR, lookup(env, which) - memory);
  outcome->element=false;

  arglist->next = outcome - memory;
  slot->value.u32 = closure - memory;
  slot->type = TYPE_FUNC;
}


int main(int argc, char ** argv)
{
  if (isatty(fileno(stdin))) {
    printf("\n    **** UNPAIR LISP v1 ****\n");
    printf("\n %lu BYTE NODE SYSTEM  ALL NODES FREE\n", sizeof(Node));
    printf("\nREADY.\n");
  }


  // Setup
  infile = stdin;
  init_node_memory();
  
  // Make placeholders for false & true
  Node * nil = new_node(TYPE_INT, 0); // must add this because index value zero is used as nil
  Node * truth = new_node(TYPE_INT, 1);
  
  // Fill placeholders (optional functionality; you can comment these out!)
  make_boolean(nil);
  make_boolean(truth);

  // ...and start using them!
  Node * env = nil;

  // REPL!
  Node * node;
  do
  {
    if (isatty(fileno(stdin))) {
      printf("> ");
    }

    node = parse();

    // In case of EOF:
    if (node == NULL) continue;

    // Compile
    node = transform(node, &env);

    // In case of compilation error:
    if (node == NULL) continue;
    
    // Since 'eval' only evaluates the first element even of a root-level list (as a convenience to our setup),
    // make a distinction here and directly invoke 'apply' separately if we have parsed such a list expression.
    // Essentially, 'eval' goes on to do the same thing for sub-lists (but distinguishes them by means of the
    // node pointer value type).
    if (node->element) node = eval(node, &env);
    else node = apply(node, & env);

    print(node);
  } while(node != NULL);
  printf("\n"); // neatly exit on a clear line
  return 0;
}
