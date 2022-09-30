# Unpair LISP

## Node design
- LHS may contain arbitrarily sized data in-line, or be of fixed size
- RHS is used for (relative) pointers only
- Nodes are flagged as either list items or elements, such that:

        elem = (value . NULL)
        list = (value . list) | (value . ())
        pair = (value . elem)

  (Where NULL and () are both empty 'next' values in a node, but the nodes are
   either marked to be an 'element' or a 'list'.)

- With the 'array' flag set, 'value' marks the array size in bytes, and the
  the data is stored in the space of what would otherwise be consecutive nodes.

Since we put values on the LHS only, what we call a 'pair' is not really a
pair; hence the name Unpair LISP. Nevertheless, if consistently applied, this
data organisation is completely transparent to the end user. The main advantages
are that all data is uniformly stored into Nodes (which is useful for GC'ing)
and type tagged troughout the system, and 'pair' notation remains supported.

## The 64-bit node
Even when extensible, the choice of node size remains a trade-off between data
width and alignment, mainly due to the extra space required by metadata. In our
node design we have left the data a neat 32-bit aligned, 32-bit value, but this
is at the cost of not having a 32-bit system pointer for 'next'; then again,
this pointer size would still not suffice on 64-bit systems.

(For those, a similar 128-bit node design would arguably be able to hold 'full'
system pointers, as most 64-bit platforms only support 48 bit addresses; but
then one trade-off would be that every single value requires 128 bits.)

The 64-bit node memory layout looks as follows:

| #Bits | Usage                          | Value range |
|-------|--------------------------------|-------------|
| 5     | Type                           | 32 types    |
| 1     | 'element' flag                 | 1           |
| 1     | 'array' flag                   | 1           |
| 1     | 'mark' GC flag                 | 1           |
| 24    | 'next' (node index) pointer    | 64MB nodes  |
| 32(+) | value (direct or node pointer) | 4-60 bytes  |


## Memory management
Storing all data inside the same unit(s), with uniform headers implies that
heap memory can be allocated as a consecutive array and is easily walked, e.g.
for garbage collection purposes. As a consequence system resources, such as
environment scopes, must be allocated in a similar fashion (which in turn
facilitates their being accessed from within the language itself). Of course,
when maintaining or walking memory, the 'array' header should be taken into
account; if set, 'value' marks the number of to follow this slot by way of
an array.

## Lambda calculus booleans
I am playing with designing the nodes representing false (= empty list, NIL)
and true (= #t) in terms of their lambda calculus equivalents, so that they
can be executed without writing 'if':

        ((< 1 2) 'yes 'no) ;; yields 'yes
        ((< 1 2) 'yes)     ;; yields 'yes
        ((> 1 2) 'yes 'no) ;; yields 'no
        ((> 1 2) 'yes)     ;; yields ()

Mind that in the absence of a 'special form', both the 'yes and 'no sides are
presently evaluated, so that:

        (define x 42)
        ((< 1 2) (set! x 3) (set! x 4) ;; yields '3', but x=4!

