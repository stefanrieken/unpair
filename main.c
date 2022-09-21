#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

typedef enum Type {
  TYPE_NONE,
  TYPE_INT,
  TYPE_STRING,
  TYPE_ID,
  TYPE_NODE,
  TYPE_FUNC
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


//
// MEMORY
//

/**
 * We must do at least a bit of our own memory management
 * on nodes, so that we may also GC them.
 */
Node * memory;
uintptr_t memsize;

// There is no 'beatiful' number for chunksize as long
// as node size = 3 * word size :)
#define chunksize 64
void init_node_memory()
{
  memory = malloc(sizeof(Node) * chunksize);
  memsize = 0;
}

Node * allocate_node()
{
  if(memsize%chunksize == 0)
  {
    memory = malloc(sizeof(Node) * (memsize+chunksize));
  }
  Node * node = &memory[memsize];
  memsize++;
  return node;
}

Node * new_node(Type type, uint32_t value)
{
  Node * node = allocate_node();
  node->size = 0; // start counting at 0 so that we may have 8 :)
  node->type = type;
  node->mark = 0;
  node->element = 1;
  node->next = 0;
  node->value.u32 = value;

  return node;
}

#define as_string(value) ((char*) &(value))

//
// PRINTING
//

void print_node(Node * node, bool recursing)
{
  if (node == NULL) return; // happens when EOF
  if (node - memory == 0) { printf("()"); return; }
  if (!node->element && !recursing) printf("(");

  switch(node->type)
  {
    case TYPE_INT: printf("%ld", node->value.i32);
      break;
    case TYPE_STRING:
    case TYPE_ID:
      printf("%s", as_string(node->value));
      break;
    case TYPE_NODE:
      print_node(&memory[node->value.u32], false);
      break;
    case TYPE_FUNC:
      printf("lambda");
      break;
  }

  if (!node->element)
  {
    if(node->next == 0) printf(")");
    else
    {
      if (memory[node->next].element)
      {
        printf(" . ");
        print_node(&memory[node->next], false);
        printf(")");
      }
      else
      {
        printf(" ");
        print_node(&memory[node->next], true);
      }
    }
  }
}

//
// PARSING
//
int buffer = -2;

int read_char()
{
  if (buffer > -2)
  { int result = buffer; buffer = -2; return result; }
  return getchar();
}

void unread(int ch)
{
  buffer = ch;
}

static inline bool is_whitespace_char(int ch)
{
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}
int read_non_whitespace_char()
{
  int ch = read_char();
  while (ch != -1 && is_whitespace_char(ch))
    ch = read_char();
  return ch;
}

Node * parse_value(int ch);

Node * parse_nodes()
{
  int ch = read_non_whitespace_char();
  if (ch == ')') return memory; // NIL

  // else
  Node * val = parse_value(ch);
  if (!val->element)
  {
    // We have parsed a full sublist as our 'value';
    // convert our value field to a pointer to that list.
    val = new_node(TYPE_NODE, val - memory);
  }

  // In all cases, we're being a list here (or a pair, or a false list - but not an atom).
  val->element = false;

  if (val != NULL)
  {
    ch = read_non_whitespace_char();
    if (ch == '.')
    {
      // supporting dotted-pair ('cons') notation.
      Node * cdr = parse_value(read_non_whitespace_char()); // may also end up being a list
      val->next = cdr - memory;
printf("CDR is element: %d\n", cdr->element);
      int ch = read_non_whitespace_char();
      if (ch != ')')
      {
        printf("Parse error: expecting ')' at end of dotted pair\n");
        exit(-1);
      }
    }
    else
    {
      unread(ch);
      val->next = parse_nodes() - memory;
    }
  }
  else printf("Parse error: null value in list(\?\?)");

  return val;
}

Node * parse_list()
{
// TODO this option is required in pairs, the other at the root;
// somehow in the middle both are fine??
//  return parse_nodes();
  return new_node(TYPE_NODE, parse_nodes() - memory);
}

Node * parse_label(int ch)
{
  if (ch == -1) return NULL;
  Node * result = new_node(TYPE_ID, 0);
  uint32_t index = result - memory;

  int idx = 0;
  result->size = 0; // +1

  while
  (
    idx < 63 // we fit 8 nodes of size 8; but leave room for '\0'
    && ch != -1 && ch != 0 && ch != ')' && ch != '.'
    && !is_whitespace_char(ch)
  )
  {
    result->value.str[idx++] = ch;
    if ((idx+4) % 8 == 0)
    {
      allocate_node(); // we're not directly using this result...
      result = &memory[index]; // (re-calibrate pointer in case memory moved)
      result->size++;  // ...instead we're expanding into it
    }

    ch = read_char();
  }

  unread(ch);

  result->value.str[idx] = '\0';

  return result;
}

Node * parse_value(int ch)
{
  if(ch == '(') return parse_list();
  else return parse_label(ch); // Everything is a label for now; refine based on actual value later.
}

//
// SPECIAL FUNCTIONS
//
Node * eval(Node * expr, Node ** env);

Node * copy(Node * node, bool recurse)
{
  Node * result = new_node(node->type, node->value.u32);
  // Allocate any overflow nodes
  for (int i=0; i< node->size; i++)
    allocate_node();

  memcpy(result, node, sizeof(Node) * (node->size+1));

  if (recurse && node->next != 0)
    result->next = copy(&memory[node->next], recurse) - memory;

  return result;
}

Node * element(Node * node)
{
  if (!node->element)
  {
    node = copy(node, false);
    node->element = true;
    node->next = 0;
  }
  return node;
}

/**
 * expr: e.g. the arguments to 'define' (simplest form), so (label, value)
 * Returns new node & makes new env pointer.
 */
Node * def_variable(Node ** env, Node * expr)
{
  if(expr == NULL) return expr;
  Node * val = eval(&memory[expr->next], env);
  Node * var = copy(expr, false);
  var->next = val - memory;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, var - memory);
  newval->element = false; // TODO is this still required after eval?
  newval->next = (*env) - memory;
  (*env) = newval;
  return val;
}

/**
 * As above, but tailored to setting lambda arguments.
 * Returns new env.
 */
Node * add_arg(Node * env, Node * name, Node * value)
{
  // Setup into a pair just like with 'def'
  // Copy this so as not to modify the expression.
  name = copy(name, false);
  name->next = element(value) - memory;

  // Chain into to environment (at front)
  Node * newval = new_node(TYPE_NODE, name - memory);
  newval->element = false;
  newval->next = env - memory;
  env = newval;
  return env;
}

Node * lookup(Node * env, Node * name)
{
  if(env == NULL || env == memory) return name; // return unresolved label

  if(strcmp(as_string(name->value), as_string(memory[env->value.u32].value)) == 0) return &memory[env->value.u32];
  return lookup(&memory[env->next], name);
}

Node * lookup_value(Node * env, Node * name)
{
  Node * var = lookup(env, name);
  if (var->next == 0) return var; // lookup returns 'name' if not found; return that.
  return &memory[var->next];
}

Node * set_variable(Node ** env, Node * expr)
{
  Node * var = lookup(*env, expr);
  Node * next = eval(&memory[expr->next], env);
  next->element = true;
  var->next = next - memory;
  return next;
}

Node * enclose(Node * env, Node * lambda)
{
  // Result: ((env) (args) (body))
  Node * closure = new_node(TYPE_FUNC, env - memory);
  closure->element=false;
  closure->next = lambda - memory;
  return closure;
}

Node * run_lambda(Node ** env, Node * lambda, Node * args)
{
  Node * lambda_env = &memory[lambda->value.u32];
  Node * argnames =   &memory[ memory[lambda->next].value.u32 ];
  Node * body =   &memory[ memory[lambda->next].next ];
  
  while (argnames != NULL && argnames != memory) // aka NIL
  {
    lambda_env = add_arg(lambda_env, argnames, eval(args, env));
    argnames = &memory[argnames->next];
    args = &memory[args->next]; // TODO should eval these (NOT in lambda_env!)
  }

  return eval(body, &lambda_env);
}
//
// EVAL / APPLY
//


// Evaluate a list expression, that is, apply function to args.
Node * apply(Node * funcexpr, Node * args, Node ** env)
{
    // Allow sub-expression at function name position
    Node * func = eval(funcexpr, env);

    if (func->type == TYPE_ID)
    {
      if(strcmp("define", func->value.str) == 0) return def_variable(env, args);
      if(strcmp("set!", func->value.str) == 0) return set_variable(env, args);
      if(strcmp("lambda", func->value.str) == 0) return enclose((*env), args);
      // else: var or special did not resolve. Maybe should return empty list.
      // Return original expression instead so that you have something to look at in REPL.
      return funcexpr;
    }
    else if (func->type == TYPE_FUNC) return run_lambda(env, func, args);
}

// Evaluate a single node, ignoring that it may be inside a list, so that:
// value => value as element(!)
// id => lookup(id)
// (..) => apply (..)
Node * eval(Node * expr, Node ** env)
{
  if (expr == NULL || expr == memory) return expr;
  if(expr->type == TYPE_ID) return lookup_value((*env), element(expr));
  if(expr->type == TYPE_NODE) return apply( &memory[expr->value.u32], &memory[ memory[expr->value.u32].next ], env);

  return element(expr);
}

int main(int argc, char ** argv)
{
  printf("sizeof Node: %lu\n", sizeof(Node));

  // Setup
  init_node_memory();
  Node * nil = new_node(TYPE_NONE, 0); // must add this because index value zero is used as nil
  Node * env = nil; // see?

  // REPL!
  Node * node;
  int ch;
  do
  {
    printf("\n> ");
    ch = read_non_whitespace_char();
    node = parse_value(ch);
    node = eval(node, &env);
    print_node(node, false);
  } while(ch != -1);
  printf("\n"); // neatly exit on a clear line
  return 0;
}
