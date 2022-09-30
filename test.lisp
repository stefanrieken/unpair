(define square (lambda (x) (* x x)))
(define quad (lambda (y) (* (square y) (square y))))
(square 42)
(quad 42)

