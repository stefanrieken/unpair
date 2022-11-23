;;
;; This library file is loaded automatically on start-up.
;;

(define list (lambda x x))

(define cadr (lambda (x) (car (cdr x))))

(define apply (lambda (fn args) (eval (cons fn args))))

(define map
  (lambda (func values)
    (if (= values '())
      '()
      (cons (func (car values)) (map func (cdr values)))
)))

(define-syntax let
  (lambda (_ vars body)
    (cons (list 'lambda (map car vars) body) (map cadr vars))
))

