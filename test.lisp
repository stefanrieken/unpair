'"Lambda definitions"
(define square (lambda (x) (* x x)))
(define quad (lambda (y) (* (square y) (square y))))

(print "42 squared is " (square 42))
(print "42 squared squared is " (quad 42))

;; Lambda-calculus style booleans(!)
(define be-amazed (lambda (x) ((> x 10) "Wow, x is larger than 10!")))
(be-amazed 20)

'"Vararg support"
((lambda x x) 1 2 3 4)
((lambda (x y . z) z) 1 2 3 4)

'"Novel list index"
;; could also have been ('(foo bar baz) 2)
(2 '(foo bar baz))

;; Macros!
;; see also e.g. 'let' in lib.lisp
'"Macros: define 'add' to mean '+' by means of a macro"
(define-syntax add (lambda (x . y) (cons '+ y)))
(add 1 2 3)

'"Functions defined in lib.lisp"
(list 1 (+ 1 1) (* 3 1))

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

