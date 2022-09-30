#ifndef PARSE_H
#define PARSE_H

#include "node.h"

// First set the (open) file
extern FILE * infile;
// Then call parse
Node * parse();

#endif /* PARSE_H */
