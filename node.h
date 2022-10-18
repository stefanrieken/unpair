#ifndef NODE_H
#define NODE_H

#include <stdint.h>

typedef enum Type {
  TYPE_INT,
  TYPE_CHAR,     // Character array,  to which String and ID (Label) point
  TYPE_STRING,  // technically, 'char'; but it makes little sense outside of a char array aka string
  TYPE_ID,      // = label
  TYPE_NODE,     // = node pointer
  TYPE_FUNC,     // = closure (transformed lambda)
  TYPE_VAR,      // references the FULL (name val) entry for pre-dereferenced variables.
  TYPE_PRIMITIVE //
} Type;

extern char * types[];

typedef struct Node {
  Type type : 5; // up to 32
  uint8_t mark: 1;  // for use by GC
  uint8_t array: 1; // value is size (always in bytes; re-interpret as needed); data is in subsequent node slots
  uint8_t element : 1;

  uint32_t next : 24; // node index 'pointer'

  union {
    int32_t i32;
    uint32_t u32;
  } __attribute__((__packed__))  value;

} __attribute__((__packed__)) Node;


#define strval(node) ((char*) (node + 1))
#define intarray(node) ((int32_t *) (node + 1))
#define uintarray(node) ((uint32_t *) (node + 1))

int length(Node * list);
int mem_usage(Node * list);

#endif /*NODE_H*/
