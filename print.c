#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "print.h"
#include "memory.h"

void print_node(Node * node, bool recursing)
{
  if (node == NULL) return; // happens when EOF
  if (node - memory == 0) { printf("()"); return; }
  if (!node->element && !recursing) printf("(");

  switch(node->type)
  {
    case TYPE_INT: printf("%ld", node->value.i32);
      break;
    case TYPE_STRING:
    case TYPE_ID:
      printf("%s", as_string(node->value));
      break;
    case TYPE_NODE:
      print_node(&memory[node->value.u32], false);
      break;
    case TYPE_FUNC:
      printf("lambda");
      break;
  }

  if (!node->element)
  {
    if(node->next == 0) printf(")");
    else
    {
      if (memory[node->next].element)
      {
        printf(" . ");
        print_node(&memory[node->next], false);
        printf(")");
      }
      else
      {
        printf(" ");
        print_node(&memory[node->next], true);
      }
    }
  }
}

