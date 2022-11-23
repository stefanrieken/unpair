#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "memory.h"
#include "parse.h"
#include "print.h"

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
  if (ch == ')') return NIL;

  // else
  Node * val = parse_value(ch);

  // In all cases, we're being a list here (or a pair, or a false list - but not an atom).
  val->element = false;

  if (val != NULL)
  {
    ch = read_non_whitespace_char();
    if (ch == '.')
    {
      // supporting dotted-pair ('cons') notation.
      Node * cdr = parse_value(read_non_whitespace_char()); // may also end up being a list
      if (cdr->type == TYPE_NODE) cdr = &memory[cdr->value.u32]; // in which case, use pointer directly, as (a . (b)) == (a b)
      val->next = index(cdr);
      int ch = read_non_whitespace_char();
      if (ch != ')')
      {
        printf("Parse error: expecting ')' at end of dotted pair\n");
        return NULL;
      }
    }
    else
    {
      unread(ch);
      val->next = index(parse_nodes());
    }
  }
  else printf("Parse error: null value in list(\?\?)");

  return val;
}

char * escapes = "nrtf";
char * replacements = "\n\r\t\f";
int escapes_length = 4;

// assumes opening quote is already parsed
Node * parse_string()
{
  Node * result = new_array_node(TYPE_CHAR, 0);

  int idx = 0;

  char * value_node = NULL;
  int chars_per_node = sizeof(Node);

  int ch = read_char();
  while
  (
    ch != -1 && ch != 0 && ch != '\"'
  )
  {
    if (ch == '\\')
    {
      ch = read_char();
      for (int i=0; i<escapes_length; i++)
        if (escapes[i] == ch) { ch = replacements[i]; break; }
    }

    if (idx % sizeof(Node) == 0) value_node = (char *) allocate_node();

    value_node[idx % chars_per_node] = ch;
    idx++;

    ch = read_char();
  }

  if (idx % sizeof(Node) == 0) value_node = (char *) allocate_node();
  value_node[idx % chars_per_node] = '\0';
  idx++;
  result->value.u32 = idx; // set size
  result->array = true;

  // Now add pointer to String result
  result = unique_string(result);
  return new_node(TYPE_STRING, index(result));
}

Node * parse_label(int ch)
{
  if (ch == -1) return NULL;
  Node * result = new_array_node(TYPE_CHAR, 0);

  int idx = 0;

  char * value_node = NULL;
  int chars_per_node = sizeof(Node);

  while
  (
    ch != -1 && ch != 0 && ch != ')' && ch != '.'
    && !is_whitespace_char(ch)
  )
  {
    if (idx % chars_per_node == 0) value_node = (char *) allocate_node();

    value_node[idx % chars_per_node] = ch;
    idx++;

    ch = read_char();
  }

  if (idx == 0)
  {
    printf("Parse error: expected label; got '%c'.\n", ch);
    return &memory[0];
  }
  // else
  unread(ch);
  if (idx % sizeof(Node) == 0) value_node = (char *) allocate_node();
  value_node[idx % chars_per_node] = '\0';
  idx++;
  result->value.u32 = idx; // set size

  // Further processing is done by parse_label_or_number
  // (depending on what it parses at)
  return result;
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

    if (intval < 0 || intval >= radix)
    {
      // Give up trying to parse label as int;
      // Return label as pointer to char array
      node = unique_string(node);
      return new_node(TYPE_ID, index(node));
    }

    result = (result * radix) + intval;
  }
  result *= sign;

  // Made it up to here; we have a number!
  // Replace character-array node value with numeric value.
  // Delete any nodes that were used to hold the string value.
  memsize -= num_value_nodes(node);

  node->type = TYPE_INT;
  node->array = false;
  if (result > INT32_MAX) node->value.u32 = result;
  else node->value.i32 = result;

  return retrofit(node);
}

/**
 * Parse the shorthand quote.
 * I'm sure that this ought to be a common label plus a LISP macro,
 * but pending a macro system it is of some value to have it built in.
 */
Node * parse_quote()
{
  Node * quote = new_node(TYPE_ID, index(unique_string(make_char_array_node("quote"))));
  quote->element = false;

  Node * val = parse();
  val->element = false;
  quote->next = index(val);
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

  if(ch == '(') return new_node(TYPE_NODE, index(parse_nodes())); // we parse one element here; so if list, wrap into node pointer
  if (ch == '\'') return new_node(TYPE_NODE, index(parse_quote()));
  if (ch == '\"') return parse_string();
  else return parse_label_or_number(ch, 10); // Everything is a label for now; refine based on actual value later.
}

Node * parse()
{
    return parse_value(read_non_whitespace_char());
}
