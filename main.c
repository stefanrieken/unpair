#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "node.h"
#include "memory.h"
#include "print.h"
#include "parse.h"
#include "eval.h"

#include "transform.h"

int main(int argc, char ** argv)
{
  printf("sizeof Node: %lu\n", sizeof(Node));

  // Setup
  init_node_memory();
  Node * nil = new_node(TYPE_NONE, 0); // must add this because index value zero is used as nil
  Node * env = nil; // see?

  // REPL!
  Node * node;
  do
  {
    printf("\n> ");
    node = parse();

    // In case of EOF:
    if (node == NULL) continue;

    if (node->element) node = transform_elem(node, &env);
    else node = transform_expr(node, & env);

    // In case of compilation error:
    if (node == NULL) continue;

    // Since 'eval' only evaluates the first element even of a root-level list (as a convenience to our setup),
    // make a distinction here and directly invoke 'apply' separately if we have parsed such a list expression.
    // Essentially, 'eval' goes on to do the same thing for sub-lists (but distinguishes them by means of the
    // node pointer value type).
    if (node->element) node = eval(node, &env);
    else node = apply(node, &memory[node->next], & env);

    print_node(node, false);
  } while(node != NULL);
  printf("\n"); // neatly exit on a clear line
  return 0;
}
