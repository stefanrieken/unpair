
/**
 * Evaluate a single element, ignoring that it may be inside a list, so that:
 * value => value as element(!)
 * id => lookup(id)
 * list pointer => apply list
 */
Node * eval(Node * expr, Node ** env);

/**
 * Evaluate a list expression, that is, apply function to args.
 */
Node * apply(Node * funcexpr, Node ** env);

Node * run_lambda(Node ** env, Node * expr, Node * args);

