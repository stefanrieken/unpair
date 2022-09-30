#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>	

#include "memory.h"
#include "parse.h"

int buffer = -2;

FILE * infile;
int read_char()
{
  if (buffer > -2)
  { int result = buffer; buffer = -2; return result; }
  return fgetc(infile);
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
        return &memory[0];
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

  char * value_node = NULL;
  int chars_per_node = sizeof(Node);

  while
  (
    ch != -1 && ch != 0 && ch != ')' && ch != '.'
    && !is_whitespace_char(ch)
  )
  {
    if (idx % sizeof(Node) == 0)
    {
      value_node = (char *) allocate_node();
      result = &memory[index]; // (re-calibrate pointer in case memory moved)
    }
    value_node[idx % chars_per_node] = ch;
    idx++;

    ch = read_char();
  }

  if (idx == 0)
  {
    printf("Parse error: expected label; got '%c'.\n", ch);
    return &memory[0];
  }
  else
  {
    unread(ch);
    value_node[idx % chars_per_node] = '\0';
    idx++;
    result->value.u32 = idx; // set size
    result->array = true;

    return result;
  }
}

Node * parse_label_or_number (int c, int radix)
{
  Node * node = parse_label(c);
  if (node == NULL) return NULL;

  char * str = strval(node);

  intptr_t result = 0;
  int sign = 1;

  int i=0;
  if (str[i] == '-' && str[i+1] != 0)
  {
    sign = -1;
    i++;
  }
  while (str[i] != 0)
  {
    int intval = str[i++] - '0'; // presently only supporting radices up to 10
    if (intval < 0 || intval >= radix) return node; // give up
    result = (result * radix) + intval;
  }
  result *= sign;

  // Made it up to here; we have a number!
  // Replace character-array node value with numeric value.
  // Delete any nodes that were used to hold the string value.
  int num_value_nodes = (node->value.u32 / sizeof(Node)) + 1;
  memsize -= num_value_nodes;

  node->type = TYPE_INT;
  node->array = false;
  if (result > INT32_MAX) node->value.u32 = result;
  else node->value.i32 = result;
  
  return node;
}

/**
 * Parse the shorthand quote.
 * I'm sure that this ought to be a common label plus a LISP macro,
 * but pending a macro system it is of some value to have it built in.
 */
Node * parse_quote()
{
  Node * quote = new_node(TYPE_ID, 6);
  char * value = (char *) allocate_node();
  strcpy (value, "quote");
  quote->element = false;

  quote->next = parse() - memory;
  return quote;
}

Node * parse_value(int ch)
{
  while (ch == ';') {
    // skip line
    do ch = read_char();
    while (ch != -1 && ch != '\n');

    ch = read_non_whitespace_char();
  }

  if(ch == '(') return parse_nodes(); else
  if (ch == '\'') return parse_quote();
  else return parse_label_or_number(ch, 10); // Everything is a label for now; refine based on actual value later.
}

Node * parse()
{
    return parse_value(read_non_whitespace_char());
}
