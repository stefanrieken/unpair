#ifndef GC_H
#define GC_H

#include "node.h"

int mark(Node * node);
Node * sweep();

#endif /*GC_H*/
