'"Lambda definitions"
(define square (lambda (x) (* x x)))

(define quad (lambda (y) (* (square y) (square y))))

'"42 squared is:"
(square 42)
'"42 squared squared is:"
(quad 42)

;; Lambda-calculus style booleans(!)
(define be-amazed (lambda (x) ((> x 10) "Wow, x is larger than 10!")))
(be-amazed 20)

;; Special arg syntax.
'"Get all args as one list variable"
((lambda x x) 1 2 3 4)

'"Scheme style &rest notation support"
((lambda (x y . z) z) 1 2 3 4)

'"Novel list index"
;; could also have been ('("a" "b" "c") 2)
(2 '("a" "b" "c"))

;; Macros!
'"Define 'add' to mean '+' by means of a macro"
(define-syntax add (lambda (x . y) (cons '+ y)))
(add 1 2 3)


;; defining 'list' doesn't even require a macro... just taking away an indirection(!)
(define list (lambda x x))
(list 1 (+ 1 1) (* 3 1))

'"Functions defined in lib.lisp"

(apply + '(1 2 3))
(apply list '(1 2 3))
;; As is common in most LISPs, macros can not be passed as arguments
;(apply add '(1 2 3))

(cadr '(1 2 3))

(map car '((1 2) (3 4)))
(map cdr '((1 2) (3 4)))

(let ((foo 33)) (+ foo foo))

'"Recursive calls finally work, thanks to proper spaghetti stack args"
(map car (map cdr '((1 2) (3 4))))

