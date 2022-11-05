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
#include "gc.h"

#include "transform.h"

Node * macros;
Node * unique_strings;

Node * make_char_array_node(char * val)
{
  Node * node = new_array_node(TYPE_CHAR, strlen(val)+1);
  char * value = strval(node);
  strcpy (value, val);
  node->element = false;
  node->array = true;
  return node;
}

Node * unique_small_string(char * val)
{
  Node * where = unique_strings;
  while (where != NIL)
  {
    if(strcmp(val, strval(where)) == 0)
      return where;
    where = &memory[where->next];
  }
  // Not found: make
  where = make_char_array_node(val);
  where->next = index(unique_strings);
  unique_strings = retrofit(where);
  return unique_strings;
}

// for 1-char args only!
Node * make_arg(char * arg)
{
  return new_node(TYPE_ID, index(unique_small_string(arg)));
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
  a->next = index(b);
  b->next = 0;

  Node * arglist = new_node(TYPE_NODE, index(a));
  arglist->element=false;

  Node * closure = new_node(TYPE_NODE, index(env));
  closure->element = false;
  closure->next = index(arglist);

  Node * which = (slot == NIL) ? b : a;
  Node * outcome = new_node(TYPE_VAR, index(lookup(env, which)));
  outcome->element=false;

  arglist->next = index(outcome);
  slot->value.u32 = index(closure);
  slot->type = TYPE_FUNC;
}

int main(int argc, char ** argv)
{
  // Setup
  infile = stdin;
  init_node_memory();
  
  // Make placeholders for false & true
  Node * nil = new_node(TYPE_NODE, 0); // must add this because index value zero is used as nil
  Node * truth = new_node(TYPE_INT, 1);  
  // ...and start using them!
  Node * env = nil;
  macros = nil;
  unique_strings = nil;
  // Fill placeholders (optional functionality; you can comment these out!)
  make_boolean(nil);
  make_boolean(truth);

  if (isatty(fileno(stdin))) {
    printf("\n     **** UNPAIR LISP v1 ****\n");
    printf("\n %lu BYTE NODE SYSTEM %ld NODES USED\n", sizeof(Node), memsize);
    printf("\nREADY.\n");
  }

  // REPL!
  Node * node;
  do
  {
    // GC
    int marked = 0;
    marked += mark(nil);
    marked += mark(truth);
    marked += mark(env);
    marked += mark(macros);
    marked += mark(unique_strings);
    freelist = sweep();

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
    if (node->element) node = eval(node, env);
    else node = apply(node, env);

    print(node);
  } while(node != NULL);
  printf("\n"); // neatly exit on a clear line
  return 0;
}
