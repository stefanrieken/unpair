#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "print.h"
#include "memory.h"
#include "primitive.h"

void print_node(Node * node, bool quote_id)
{
  switch(node->type)
  {
    case TYPE_NONE: printf("(none)");
      break;
    case TYPE_INT: printf("%d", node->value.i32);
      break;
    case TYPE_STRING:
    case TYPE_ID:
      if(quote_id) printf("'");
      printf("%s", strval(node));
      break;
    case TYPE_NODE:
      printf("(");
      print_node(&memory[node->value.u32], quote_id);
      printf(")");
      break;
    case TYPE_FUNC:
      printf("(lambda ");
      print_node(&memory[memory[node->value.u32].next], false);
      printf(")");
      break;
    case TYPE_VAR:
      printf("%s", strval(&memory[node->value.u32]));
      break;
    case TYPE_PRIMITIVE:
      printf("%s", primitives[node->value.u32]);
      break;
  }

  if (!node->element && !node->next == 0)
  {
    if (memory[node->next].element)
    {
      printf(" . ");
      print_node(&memory[node->next], true);
    }
    else
    {
      printf(" ");
      print_node(&memory[node->next], true);
    }
  }
}

void print(Node * node)
{
  if (node == NULL) return; // happens when EOF
  if(node == NIL) {printf("()\n"); return;}
  if(node == NIL+1) {printf("#t\n"); return;}

  if (!node->element) printf("(");
  print_node(node, true);
  if (!node->element) printf(")");
  printf("\n");
}
