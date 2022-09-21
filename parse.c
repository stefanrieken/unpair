#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "memory.h"
#include "parse.h"

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
  if(ch == '(') return parse_nodes();
  else return parse_label(ch); // Everything is a label for now; refine based on actual value later.
}

Node * parse()
{
    return parse_value(read_non_whitespace_char());
}
