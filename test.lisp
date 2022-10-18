
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

'"Apply"
(define-syntax apply (lambda x (cdr x)))
;; Works with primitives, user funcs AND user macros
(apply + 1 2 3)
(apply list 1 2 3)
(apply add 1 2 3)

'"Cadr"
(define-syntax cadr
  (lambda (_ val)
    (list 'car (list 'cdr val))))

(cadr '(1 2 3))
'"Map function"
;; This function is self-recursive, which doesn't quite work yet:
;; replace the failing (map func (cdr values)) with (car (cdr values))
;; (or the reverse) to confirm that this is the problem spot.
(define map
  (lambda (func values)
    (if (= values '())
      '()
      (cons (func (car values)) (map func (cdr values)))
)))

(define map2
  (lambda (func values)
    (if (= values '())
      '()
      (cons (func (car values)) (map func (cdr values)))
)))

;; TODO this doesn't work, because
;; of the recursive use of the same argument slots.
;(map car (map cdr '((1 2) (3 4))))
;; I say that because this works:
(map2 car (map cdr '((1 2) (3 4))))

;; cannot presently map a macro in
;(map cadr '((1 . 2) (3 . 4)))

;; This one is also still in the making
;(define-syntax let
;  (lambda (_ vars body)
;    (list (list 'lambda (map car vars) body) (map cdr vars))
;))

;(let ((foo 33)) (+ foo foo))

