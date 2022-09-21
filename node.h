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
  uint8_t size : 3;
  Type type : 3;
  uint8_t mark: 1;
  uint8_t element : 1; // TBD

  uint32_t next : 24; // node index 'pointer'

  union {
    int32_t i32;
    uint32_t u32;
    char str[4];
  } __attribute__((__packed__))  value;

} __attribute__((__packed__)) Node;

#define as_string(value) ((char*) &(value))

#endif /*NODE_H*/
