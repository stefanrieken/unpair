#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "node.h"
#include "memory.h"
#include "print.h"

//
// MEMORY
//

/**
 * We must do at least a bit of our own memory management
 * on nodes, so that we may also GC them.
 */
Node * memory;
Node * freelist;
uintptr_t memsize;

#define chunksize 1024

void init_node_memory()
{
  memory = malloc(sizeof(Node) * chunksize);
  memsize = 0;
  freelist = NIL;
}

Node * init_node(Node * node, Type type, uint32_t value, bool array)
{
  node->array = array;
  node->type = type;
  node->mark = false;
  node->element = true;
  node->next = 0;
  node->value.u32 = value;

  return node;
}

/**
 * Resize a (free) node into something smaller,
 * and chain in the newly created remainder node.
 */
int resize(Node * node, int oldsize, int newsize)
{
  // Say oldsize = 3, newsize = 2:
  int node2size = oldsize - newsize;

  Node * node2 = node + newsize;

  // This part should in theory be unnecessary, as
  // the very next thing done is copying in the data.
  if (newsize > 1)
  {
     node->array = true;
     node->value.u32 = (newsize - 1) * sizeof(Node);
  }
  else node->array = false;

  // But this part is required
  if (node2size > 1)
  {
     node2->array = true;
     node2->value.u32 = (node2size - 1) * sizeof(Node);
  }
  else node2->array = false;


  node2->next = node->next;
  node->next = idx(node2);

  return newsize;
}

/**
 * Allocate un-initialized node space at end of memory.
 */
Node * allocate_node()
{
  if(memsize%chunksize == 0)
  {
    Node * mem_old = memory;
    memory = realloc(memory, sizeof(Node) * (memsize+chunksize));
    if (mem_old != memory) printf("WARN: memory moved (memsize=%ld). This invalidates all pointers, which we presently can't handle!\n", memsize);
  }
  Node * node = &memory[memsize];
  memsize++;
  return node;
}

/**
 * Return a fixed-sized node, either from
 * reclaimed memory or fully new.
 */
Node * new_node(Type type, uint32_t value)
{
  // Uncomment to temporarily disable memory reclamation.
  //return init_node(allocate_node(), type, value);

  Node * before = NIL;
  Node * reclaimable = freelist;
  // Be lazy and preserve free array entries for re-use as arrays
  while (reclaimable != NIL && reclaimable->array)
  {
    before = reclaimable;
    reclaimable = addr(reclaimable->next);
  }

  //return init_node(allocate_node(), type, value);
  
  Node * result;
  if (reclaimable != NIL)
  {
    //printf("Reclaiming %ld %d\n", reclaimable - memory, reclaimable->next);
    if(before != NIL) before->next = reclaimable->next; // unlink item from freelist
    else freelist = &memory[freelist->next]; // at start of freelist; move one up
    result = reclaimable;
  }
  else result = allocate_node();
  
  return init_node(result, type, value, false);
}

/**
 * Return a stretchable node at end of memory.
 * Allocates the node space required to host the amount of bytes
 * marked by 'value'.
 */
Node * new_array_node(Type type, uint32_t value)
{
  Node * result = init_node(allocate_node(), type, value, true);
  // Allocate any overflow nodes
  for (int i=0; i< num_value_nodes(result); i++)
    allocate_node();
  return result;
}

// Try to move the last created note into an existing slot.
// This approach is useful because we want an unlimited array
// for parsing (even integers) first.
Node * retrofit (Node * node)
{
  // Uncomment to temporarily disable retrofitting.
  //return node;

  Node * available = freelist;
  Node * before = NIL;

  int size_required = 1;
  if(node->array) size_required += num_value_nodes(node);

  if ((node-memory) + size_required != memsize) printf("Strange! %ld %ld\n", (node-memory)+size_required, memsize);
  recurse:
 
  if (available == NIL) return node;

  int size_available = 1;
  if(available->array) size_available += num_value_nodes(available);

  if (size_available > size_required)
    size_available = resize(available, size_available, size_required);
  
  if (size_available == size_required)
  {
    //printf("Retrofitting; size=%d\n", size_required);print(node);
    if (before != NIL) before->next = available->next; // unchain result from freelist
    else  freelist = &memory[freelist->next]; // at start of freelist; just move it one up

    Node * result = available;
    memcpy(result, node, sizeof(Node) * size_required);
    memsize -= size_required; // yay, successfully reduced memsize using GC!

    return result;
  }
  
  // else
  //printf("(%d != %d)", size_available, size_required);
  before = available;
  available = &memory[available->next];
 
  goto recurse;
}

/**
 * n_recurse:
 * - 0 = none
 * - n = n times
 * - -n = indefinitely (or until int wraps around...)
 */
Node * copy(Node * node, int n_recurse)
{
  if (node == NULL) return NULL;

  // Since 'new_node' presently reclaims single nodes only,
  // just allocate space for copying arrays at end.
  // Though sub-optimal as a final solution,
  // we can for now try to retrofit the result.
  Node * result = node->array ? new_array_node(node->type, node->value.u32) : new_node(node->type, node->value.u32);

  int num_nodes = 1;
  if (node->array) num_nodes += num_value_nodes(node);
  memcpy(result, node, sizeof(Node) * num_nodes);

  if (n_recurse > 0 && node->next != 0)
    result->next = copy(&memory[node->next], n_recurse-1) - memory;
  else
    result->next = 0; // TODO this is unexpected behaviour in some cases

  // Only try retrofit if we know the result is at top of memory!
  if (node->array) return retrofit(result);
  else return result;
}

/**
 * If the node is not yet an element,
 * make an element copy.
 */
Node * element(Node * node)
{
  if (!node->element)
  {
    node = copy(node, 0);
    node->element = true;
    node->next = 0;
  }
  return node;
}

/**
 * Returns the 'raw' 'env' entry, to be refined by the specializations below.
 */
Node * lookup_internal(Node * env, Node * name)
{
  while (env != NULL && env != NIL)
  {
    Node * envnode = &memory[env->value.u32];
    // Thanks to unique label character arrays, we can now just compare pointers here
    if (envnode->value.u32 == name->value.u32) return env;

    env = &memory[env->next];
  }

  // Return either NULL if no env; or NIL if not found
  return env;
}

/**
 * Lookup a variable. Return NIL if not found.
 */
Node * lookup(Node * env, Node * name)
{
  Node * result = lookup_internal(env, name);
  if(result == NULL || result == NIL)
  {
    // This should be normally caught as a compile time error
    printf("Runtime error: cannot find variable '%s'\n", strval(name));
    return NIL; // by means of recovery??
  }
  // else
  return &memory[result->value.u32];
}


/**
 * Lookup a variable; return a TYPE_VAR reference, or NIL if not found.
 *
 * NOTE: presently the TYPE_VAR reference is a bit of a silly wrapper around the
 * variable in memory. The longer-term goal is to encode TYPE_VAR in such a way
 * that the actual variable can be found quickly even in dynamic memory circumstances;
 * e.g. x = 0,1 for variable idx 1 in the top-level env.
 */
Node * dereference(Node * env, Node * name)
{
  Node * result = lookup_internal(env, name);
  if(result == NULL || result == NIL) return NIL; // return unresolved label
  // else
  return new_node(TYPE_VAR, result->value.u32);

}

// Shameless copy of lookup from eval, for the purpose of macro lookups.
// Should of course be (further) merged.
Node * find_macro(Node * env, Node * name)
{
  Node * result = lookup_internal(env, name);
  if(result == NULL || result == NIL) return NIL; // return unresolved label
  // else
  return &memory[memory[result->value.u32].next];
}

