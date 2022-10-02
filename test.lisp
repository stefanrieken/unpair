
;; Lambda definitions
(define square (lambda (x) (* x x)))
(gc)

(define quad (lambda (y) (* (square y) (square y))))

;; Feel the power of the garbage collector!
;; Try commenting this call to 'gc' out, and see how
;; this affects memsize in the later stats.
;(gc)

'"42 squared is:"
(square 42)
'"42 squared squared is:"
(quad 42)

;; Lambda-calculus style booleans(!)
;; "Wow, x is larger than 10!"
;; "Tsjonge jonge jonge hee"
(define be-amazed (lambda (x) ((> x 10) "Wow, x is larger than 10!")))
(be-amazed 20)

;; TODO somehow the gc stats are off by 4 on the first run only
;; Hint: these are the nodes from the below quoted-list header.
'(memsize used free)
(gc)
(gc)
(gc)

