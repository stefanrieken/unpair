#ifndef GC_H
#define GC_H

#include "node.h"

int mark(Node * node);
Node * sweep(Node * freelist, uint32_t start);

#endif /*GC_H*/
