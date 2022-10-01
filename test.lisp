;; Lambda definitions
(define square (lambda (x) (* x x)))
(define quad (lambda (y) (* (square y) (square y))))
'"42 squared is:"
(square 42)
'"42 squared squared is:"
(quad 42)

;; Lambda-calculus style booleans(!)
(define be-amazed (lambda (x) ((> x 10) "Wow, x is larger than 10!")))
(be-amazed 20)

;; TODO somehow the gc stats are off by 4 on the first run only
'(memsize used free)
(gc)
(gc)
(gc)

