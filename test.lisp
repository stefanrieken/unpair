;; Lambda definitions
(define square (lambda (x) (* x x)))
(define quad (lambda (y) (* (square y) (square y))))
(square 42)
(quad 42)

;; Lambda-calculus style booleans(!)
(define be-amazed (lambda (x) ((> x 10) 'wow-large)))
(be-amazed 20)

;; TODO somehow the gc stats are off by 4 on the first run only
'(memsize used free)
(gc)
(gc)
(gc)

