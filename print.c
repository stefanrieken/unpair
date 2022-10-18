#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "print.h"
#include "memory.h"
#include "primitive.h"

void print_node(Node * node)
{
  switch(node->type)
  {
    case TYPE_INT: printf("%d", node->value.i32);
      break;
    case TYPE_CHAR:
      if(node->array)
      {
        printf("[%s]", strval(node));
      }
      else
        printf("'%uc'", (unsigned char) node->value.u32);
      break;
    case TYPE_STRING:
      printf("\"%s\"", strval(&memory[node->value.u32]));
      break;
    case TYPE_ID:
      printf("%s", strval(&memory[node->value.u32]));
      break;
    case TYPE_NODE:
      printf("(");
      if (node->value.u32 != 0) print_node(&memory[node->value.u32]);
      printf(")");
      break;
    case TYPE_FUNC:
      printf("(lambda ");
      print_node(&memory[memory[node->value.u32].next]);
      printf(")");
      break;
    case TYPE_VAR:
      printf("%s", strval(&memory[memory[node->value.u32].value.u32]));
      break;
    case TYPE_PRIMITIVE:
      printf("%s", primitives[node->value.u32]);
      break;
  }

  if (node->next != 0)
  {
    if (memory[node->next].element)
      printf(" . ");
    else printf(" ");

    print_node(&memory[node->next]);
  }
}

// Really is 'print element'
void print(Node * node)
{
  if (node == NULL) return; // happens when EOF
  if(node == NIL) {printf("()\n"); return;}
  if(node == NIL+1) {printf("#t\n"); return;}

  //if (node->type == TYPE_NODE) printf("{");
  print_node(node);
  //if (node->type == TYPE_NODE) printf("}");
  printf("\n");
}
