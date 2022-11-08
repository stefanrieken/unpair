#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "node.h"

extern Node * macros;

Node * transform(Node * expr, Node ** constructing_env, Node * existing_env);

Node * transform_elements(Node * els, Node ** constructing_env, Node * existing_env);

Node * transform_elem(Node * elem, Node ** constructing_env, Node * existing_env);
Node * transform_expr(Node * expr, Node ** constructing_env, Node * existing_env);

#endif /* TRANSFORM_H */
