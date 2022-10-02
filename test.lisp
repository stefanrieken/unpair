
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
'"Scheme style &rest notation support"
((lambda (x y . z) z) 1 2 3 4)

