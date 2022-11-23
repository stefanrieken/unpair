#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "node.h"
#include "memory.h"

#include "parse.h"
#include "transform.h"
#include "eval.h"
#include "print.h"

#include "gc.h"
#include "primitive.h"

// An attempt at lambda-calculus style boolean values.
// They are at memory locations 0 (false, empty list, NIL) and 1 (true)
// TODO: since these are normal, (non-lazy / special) functions,
// both the 'true' and 'false' arguments are evaluated, so presently
// this cannot replace a complete 'if' implementation.
void make_boolean(Node * slot)
{
  Node * env = NIL;

  Node * a = unique_string(make_char_array_node("a"));
  Node * b = unique_string(make_char_array_node("b"));

  Node * args = chain(TYPE_ID, index(a),
                chain(TYPE_ID, index(b),
                NIL
  ));

  Node * body = slot == NIL ? b : a;

  Node * lambda = chain(TYPE_NODE, index(args),
                  chain(TYPE_ID , index(body),
                  NIL
  ));

  Node * enclosed = enclose(lambda, &env);
  // Copy result into exisiting 'true' / 'false' slot.
  // This immediately wastes the wrapper node returned by 'enclose'.
  // We could split off a variant of 'enclose' that doesn't return
  // a (properly) wdrapped element instead.
  slot->value.u32 = enclosed->value.u32;
  slot->type = enclosed->type;
}

Node * nil;
Node * truth;
Node * environment;
Node * macros;
Node * unique_strings;

void repl(FILE * file, bool show_results)
{
  // REPL!
  infile = file;
  bool interactive = isatty(fileno(infile));

  Node * node;
  do
  {
    // GC
    int marked = 0;
    marked += mark(nil);
    marked += mark(truth);
    marked += mark(environment);
    marked += mark(macros);
    marked += mark(unique_strings);
    freelist = sweep();

    if (interactive) {
      printf("> ");
    }

    node = parse();
    // In case of EOF:
    if (node == NULL) continue;

    // Compile
    node = transform(node, &environment, environment);
    // In case of compilation error:
    if (node == NULL) continue;

    if (!node->special) node = eval(node, environment);

    if (show_results) print(node);

  } while(node != NULL);
}

int main(int argc, char ** argv)
{
  // Setup
  init_node_memory();

  // Make placeholders for false & true
  nil = new_node(TYPE_NODE, 0); // must add this because index value zero is used as nil
  nil->element = false;
  truth = new_node(TYPE_INT, 1);
  // ...and start using them!
  environment = nil;
  macros = nil;
  unique_strings = nil;
  // Fill placeholders (optional functionality; you can comment these out!)
  make_boolean(nil);
  make_boolean(truth);

  FILE * lib = fopen("lib.lisp", "r");
  repl(lib, false);
  fclose(lib);

  if (isatty(fileno(stdin))) {
    printf("\n     **** UNPAIR LISP v1 ****\n");
    printf("\n %lu BYTE NODE SYSTEM %ld NODES USED\n", sizeof(Node), memsize);
    printf("\nREADY.\n");
  }

  repl(stdin, true);

  printf("\n"); // neatly exit on a clear line
  return 0;
}
