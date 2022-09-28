typedef Node * (* PrimitiveCb) (Node * args, Node ** env);

extern char * primitives[];
extern PrimitiveCb jmptable[];

int find_primitive(char * name);
