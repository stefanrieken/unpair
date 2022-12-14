#ifndef NODE_H
#define NODE_H

#include <stdint.h>

typedef enum Type {
  TYPE_INT,
  TYPE_CHAR,     // Character array,  to which String and ID (Label) point
  TYPE_STRING,   // technically, 'char'; but it makes little sense outside of a char array aka string
  TYPE_ID,       // = label
  TYPE_NODE,     // pointer to node; if not an element (usually) = pointer to list (=data; but may turn into code later)
  //TYPE_EXPR,     // pointer to code (an expression)
  TYPE_FUNC,     // = closure (transformed lambda)
  TYPE_ARG,      // references a per-instance variable (function argument or local 'define')
  TYPE_VAR,      // references the FULL (name val) entry for pre-dereferenced variables.
  TYPE_PRIMITIVE //
} Type;

extern char * types[];

typedef struct Node {
  Type type : 4; // up to 16
  uint8_t mark: 1;  // for use by GC
  uint8_t array: 1; // value is size (always in bytes; re-interpret as needed); data is in subsequent node slots
  uint8_t element : 1;
  uint8_t special : 1;

  uint32_t next : 24; // node index 'pointer'

  union {
    int32_t i32;
    uint32_t u32;
  } __attribute__((__packed__))  value;

} __attribute__((__packed__)) Node;


#define strval(node) ((char*) (node + 1))
#define intarray(node) ((int32_t *) (node + 1))
#define uintarray(node) ((uint32_t *) (node + 1))
#define nodearray(node) ((Node *) (node + 1))

#define pointer_to(pointed) new_node(TYPE_NODE, index(pointed))

int length(Node * list);
int mem_usage(Node * list);

// Basically 'cons' with explicit type
Node * chain(Type type, uint32_t value, Node * cdr);

#endif /*NODE_H*/
