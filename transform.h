#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "node.h"

Node * transform_elem(Node * elem, Node ** env);
Node * transform_expr(Node * expr, Node ** env);

#endif /* TRANSFORM_H */
