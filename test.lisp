
;; Lambda definitions
(define square (lambda (x) (* x x)))
(gc)

(define quad (lambda (y) (* (square y) (square y))))

'"42 squared is:"
(square 42)
'"42 squared squared is:"
(quad 42)

;; Lambda-calculus style booleans(!)
(define be-amazed (lambda (x) ((> x 10) "Wow, x is larger than 10!")))
(be-amazed 20)

;; GC; this is now actually also done in the REPL loop
;; but the call can still be used to get stats.
;; Do not call from inside a sub-expression!
'(memsize used free)
(gc)

;; Special arg syntax.
'"Get all args as one list variable"
((lambda x x) 1 2 3 4)
(gc)
'"Scheme style &rest notation support"
((lambda (x y . z) z) 1 2 3 4)
(gc)

;; Macros!
'"Define 'add' to mean '+' by means of a macro"
(define-syntax add (lambda (x . y) (cons '+ y)))
(add 1 2 3)


;; defining 'list' doesn't even require a macro... just taking away an indirection(!)
(define list (lambda x x))
(list 1 (+ 1 1) (* 3 1))

(define-syntax apply (lambda x (cdr x)))
;; Works with primitives, user funcs AND user macros
(apply '+ 1 2 3)
(apply 'list 1 2 3)
(apply 'add 1 2 3)

(define-syntax cadr
  (lambda (_ val)
    (list 'car (list 'cdr val))))

(define-syntax let
  (lambda (bla vars body)
    (list (list 'lambda (list (car vars)) body) (cadr vars))
))

(let (foo 33) (+ foo foo))

