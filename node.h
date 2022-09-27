#ifndef NODE_H
#define NODE_H

typedef enum Type {
  TYPE_NONE,
  TYPE_INT,
  TYPE_STRING,
  TYPE_ID, // 'label'
  TYPE_NODE, // = node pointer
  TYPE_FUNC // holds the lambda environment as first element, so is also used to distinguish lambda
} Type;

typedef struct Node {
  Type type : 5; // up to 32
  uint8_t mark: 1;
  uint8_t array: 1; // value is size (always in bytes; re-interpret as needed); data is in subsequent node slots
  uint8_t element : 1;

  uint32_t next : 24; // node index 'pointer'

  union {
    int32_t i32;
    uint32_t u32;
  } __attribute__((__packed__))  value;

} __attribute__((__packed__)) Node;

#define strval(node) ((char*) (node + 1))

#endif /*NODE_H*/
