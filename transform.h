#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "node.h"

extern Node * macros;

Node * transform(Node * expr, Node ** env);

Node * transform_elements(Node * els, Node ** env);

Node * transform_elem(Node * elem, Node ** env);
Node * transform_expr(Node * expr, Node ** env);

#endif /* TRANSFORM_H */
