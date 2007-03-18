;; ops.lisp

;; MathMap

;; Copyright (C) 2004-2007 Mark Probst

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program; if not, you can either send email to this
;; program's maintainer or write to: The Free Software Foundation,
;; Inc.; 675 Massachusetts Avenue; Cambridge, MA 02139, USA.

(defparameter *operators* nil)

(defparameter *types* '((nil         nil            ("NIL" "NIL" ()))
			(int        "int"           ("%d" "%d" ("~A")))
			(float      "float"         ("%f" "%f" ("~A")))
			(complex    "complex float" ("%f + %f i"
						     "COMPLEX(%f,%f)"
						     ("crealf(~A)" "cimagf(~A)")))
			(color      "color_t"       ("(%d,%d,%d,%d)" "MAKE_RGBA_COLOR(%d,%d,%d,%d)"
						     ("RED(~A)" "GREEN(~A)" "BLUE(~A)" "ALPHA(~A)")))
			(gsl-matrix "gsl_matrix *"  ("***MAXTRIX***" "***MATRIX***" ()))
			(v2         "mm_v2_t"       ("[%f,%f]" "MAKE_V2(%f,%f)"
						     ("~A.v[0]" "~A.v[1]"))
				    ("v[0]" "v[1]"))
			(v3         "mm_v3_t"       ("[%f,%f,%f]" "MAKE_V3(%f,%f,%f)"
						     ("~A.v[0]" "~A.v[1]" "~A.v[2]"))
				    ("v[0]" "v[1]" "v[2]"))
			(m2x2       "mm_m2x2_t"     ("[[%f,%f],[%f,%f]]" "MAKE_M2X2(%f,%f,%f,%f)"
						     ("~A.a00" "~A.a01" "~A.a10" "~A.a11"))
				    ("a00" "a01" "a10" "a11"))))

(defun type-with-name (name)
  (assoc name *types*))

(defparameter *int-type* (type-with-name 'int))
(defparameter *float-type* (type-with-name 'float))
(defparameter *complex-type* (type-with-name 'complex))

(defparameter *max-float-types* (list *int-type* *float-type*))
(defparameter *max-types* (list *int-type* *float-type* *complex-type*))

(defun type-name (type)
  (first type))

(defun type-c-define (type)
  (format nil "TYPE_~A" (ucs (first type))))

(defun type-c-type (type)
  (second type))

(defun type-print-info (type)
  (third type))

(defun type-elements (type)
  (fourth type))

(defmacro defop (name arity c-name
		      &key (c-define nil) (type-prop 'const) (type 'float) (pure t) (foldable t) (arg-type 'float))
  (let ((c-define (if (null c-define)
		    (string-concat "OP_" (ucs name))
		  c-define)))
    `(push '(,name ,arity ,c-define ,c-name ,type-prop ,(type-with-name type) ,pure ,(if pure foldable nil) ,arg-type)
	   *operators*)))

(defop nop 0 "NOP" :type int)

(defop int-to-float 1 "INT2FLOAT" :type float :arg-type int)
(defop int-to-complex 1 "INT2COMPLEX" :type complex :arg-type int)
(defop float-to-complex 1 "FLOAT2COMPLEX" :type complex :arg-type float)

(defop + 2 "ADD" :type-prop max :type nil :c-define "OP_ADD")
(defop - 2 "SUB" :type-prop max :type nil :c-define "OP_SUB")
(defop - 1 "NEG" :type-prop max :type nil :c-define "OP_NEG")
(defop * 2 "MUL" :type-prop max :type nil :c-define "OP_MUL")
(defop / 2 "DIV" :c-define "OP_DIV")
(defop % 2 "MOD" :c-define "OP_MOD")

(defop abs 1 "fabs" :type-prop max-float :type nil)
(defop min 2 "MIN" :type-prop max-float :type nil)
(defop max 2 "MAX" :type-prop max-float :type nil)

(defop sqrt 1 "sqrt")
(defop hypot 2 "hypot")
(defop sin 1 "sin")
(defop cos 1 "cos")
(defop tan 1 "tan")
(defop asin 1 "asin")
(defop acos 1 "acos")
(defop atan 1 "atan")
(defop atan2 2 "atan2")
(defop pow 2 "pow")
(defop exp 1 "exp")
(defop log 1 "log")
(defop sinh 1 "sinh")
(defop cosh 1 "cosh")
(defop tanh 1 "tanh")
(defop asinh 1 "asinh")
(defop acosh 1 "acosh")
(defop atanh 1 "atanh")
(defop gamma 1 "GAMMA")

(defop floor 1 "floor" :type int)
(defop = 2 "EQ" :type int :c-define "OP_EQ")
(defop < 2 "LESS" :type int :c-define "OP_LESS")
(defop <= 2 "LEQ" :type int :c-define "OP_LEQ")
(defop not 1 "NOT" :type int)

(defop print 1 "PRINT_FLOAT" :type int :pure nil)
(defop newline 0 "NEWLINE" :type int :pure nil)

(defop start-debug-tuple 1 "START_DEBUG_TUPLE" :type int :pure nil)
(defop set-debug-tuple-data 2 "SET_DEBUG_TUPLE_DATA" :type int :pure nil)

(defop orig-val 4 "ORIG_VAL" :type color :foldable nil)
(defop red 1 "RED_FLOAT" :foldable nil)
(defop green 1 "GREEN_FLOAT" :foldable nil)
(defop blue 1 "BLUE_FLOAT" :foldable nil)
(defop alpha 1 "ALPHA_FLOAT" :foldable nil)

(defop complex 2 "COMPLEX" :type complex)
(defop c-real 1 "crealf" :arg-type complex)
(defop c-imag 1 "cimagf" :arg-type complex)
(defop c-sqrt 1 "csqrtf" :type complex :arg-type complex)
(defop c-sin 1 "csinf" :type complex :arg-type complex)
(defop c-cos 1 "ccosf" :type complex :arg-type complex)
(defop c-tan 1 "ctanf" :type complex :arg-type complex)
(defop c-asin 1 "casinf" :type complex :arg-type complex)
(defop c-acos 1 "cacosf" :type complex :arg-type complex)
(defop c-atan 1 "catanf" :type complex :arg-type complex)
(defop c-pow 2 "cpowf" :type complex :arg-type complex)
(defop c-exp 1 "cexpf" :type complex :arg-type complex)
(defop c-log 1 "clogf" :type complex :arg-type complex)
(defop c-arg 1 "cargf" :arg-type complex)
(defop c-sinh 1 "csinhf" :type complex :arg-type complex)
(defop c-cosh 1 "ccoshf" :type complex :arg-type complex)
(defop c-tanh 1 "ctanhf" :type complex :arg-type complex)
(defop c-asinh 1 "casinhf" :type complex :arg-type complex)
(defop c-acosh 1 "cacoshf" :type complex :arg-type complex)
(defop c-atanh 1 "catanhf" :type complex :arg-type complex)
(defop c-gamma 1 "cgamma" :type complex :arg-type complex)

(defop ell-int-k-comp 1 "ELL_INT_K_COMP")
(defop ell-int-e-comp 1 "ELL_INT_E_COMP")

(defop ell-int-f 2 "ELL_INT_F")
(defop ell-int-e 2 "ELL_INT_E")
(defop ell-int-p 3 "ELL_INT_P")
(defop ell-int-d 3 "ELL_INT_D")

(defop ell-int-rc 2 "ELL_INT_RC")
(defop ell-int-rd 3 "ELL_INT_RD")
(defop ell-int-rf 3 "ELL_INT_RF")
(defop ell-int-rj 4 "ELL_INT_RJ")

(defop ell-jac 2 "ELL_JAC" :type v3)

(defop make-m2x2 4 "MAKE_M2X2" :type m2x2)
(defop make-m3x3 9 "MAKE_M3X3" :type m3x3 :pure nil)
(defop free-matrix 1 "FREE_MATRIX" :type int :pure nil)

(defop make-v2 2 "MAKE_V2" :type v2)
(defop make-v3 3 "MAKE_V3" :type v3)
(defop vector-nth 2 "VECTOR_NTH" :foldable nil)

(defop solve-linear-2 2 "SOLVE_LINEAR_2" :type v2 :pure nil)
(defop solve-linear-3 2 "SOLVE_LINEAR_3" :type v3 :pure nil)

(defop solve-poly-2 3 "SOLVE_POLY_2" :type v2 :pure nil)
(defop solve-poly-3 4 "SOLVE_POLY_3" :type v3 :pure nil)

(defop noise 3 "noise")
(defop rand 2 "RAND" :pure nil)

(defop userval-int 1 "USERVAL_INT_ACCESS" :type int :foldable nil)
(defop userval-float 1 "USERVAL_FLOAT_ACCESS" :type float :foldable nil)
(defop userval-bool 1 "USERVAL_BOOL_ACCESS" :type int :foldable nil)
(defop userval-curve 2 "USERVAL_CURVE_ACCESS" :type float :foldable nil)
(defop userval-color 1 "USERVAL_COLOR_ACCESS" :type color :foldable nil)
(defop userval-gradient 2 "USERVAL_GRADIENT_ACCESS" :type color :foldable nil)

(defop make-color 4 "MAKE_COLOR" :type color)
(defop output-color 1 "OUTPUT_COLOR" :type int :pure nil)

(defun max-type (types)
  (cond ((null types)
	 (error "cannot determine max type of empty list"))
	((null (cdr types))
	 (car types))
	(t
	 (let ((rest-max (max-type (cdr types))))
	   (cond ((eq (car types) *complex-type*)
		  *complex-type*)
		 ((eq (car types) *int-type*)
		  rest-max)
		 (t
		  (assert (eq (car types) *float-type*))
		  (if (eq rest-max *complex-type*)
		      *complex-type*
		    *float-type*)))))))

(defun make-op-defines ()
  (apply #'string-concat
	 (mapcar #'(lambda (op index)
		     (format nil "#define ~A ~A~%" (third op) index))
		 (reverse *operators*) (integers-upto (length *operators*)))))

(defun make-init-ops ()
  (apply #'string-concat
	 (mapcar #'(lambda (op)
		     (destructuring-bind (name arity c-define c-name type-prop type pure foldable arg-type-name)
			 op
		       (format nil "    init_op(~A, \"~A\", ~A, TYPE_PROP_~A, ~A, ~:[0~;1~], ~:[0~;1~]);~%"
			       c-define c-name arity (ucs type-prop)
			       (type-c-define type) pure foldable)))
		 (reverse *operators*))))

(defun make-op-folders ()
  (apply #'string-concat
	 (mapcar #'(lambda (op)
		     (destructuring-bind (name arity c-define c-name type-prop type pure foldable arg-type-name)
			 op
		       (labels ((switch-args (arg-types)
				  (if (= (length arg-types) arity)
				      (let ((max-type (max-type arg-types)))
					(format nil "return make_~A_const_primary(~A(~{(~A)rhs->v.op.args[~A].v.~A_const~^, ~}));~%"
						(dcs (type-name max-type)) c-name
						(mappend #'(lambda (i arg-type)
							     (list (type-c-type max-type) i (dcs (type-name arg-type))))
							 (integers-upto arity) arg-types)))
				      (format nil "switch (rhs->v.op.args[~A].const_type) {~%~Adefault : assert(0); break;~%}~%"
					      (length arg-types)
					      (apply #'string-concat
						     (mapcar #'(lambda (type)
								 (format nil "case ~A :~%~A"
									 (type-c-define type)
									 (switch-args (append arg-types (list type)))))
							     (if (eq type-prop 'max)
								 *max-types*
								 *max-float-types*)))))))
			 (if foldable
			     (if (eq type-prop 'const)
				 (format nil "case ~A :~%return make_~A_const_primary(~A(~{OP_CONST_~A_VAL(~A)~^, ~}));~%"
					 c-define (dcs (type-name type)) c-name
					 (mappend #'(lambda (i)
						      (list (ucs arg-type-name) i))
						  (integers-upto arity)))
				 (format nil "case ~A :~%~A"
					 c-define
					 (switch-args '())))
			     ""))))
		 (reverse *operators*))))

(defun make-types-file ()
  (with-open-file (out "compiler_types.h" :direction :output :if-exists :supersede)
    (format out "~{#define ~A ~A~%~}"
	    (mappend #'(lambda (type index)
			 (list (type-c-define type) index))
		     *types* (integers-upto (length *types*))))
    (format out "~%#define MAX_TYPE ~A~%~%" (type-c-define (car (last *types*))))
    (format out "#define PRIMARY_CONST_DECLS ~{~A ~A_const;~^ ~}~%~%"
	    (mappend #'(lambda (type)
			 (let ((c-type (type-c-type type)))
			   (if (null c-type)
			       '()
			       (list c-type (dcs (type-name type))))))
		     *types*))
    (format out "static char*~%type_c_type_name (int type)~%{~%switch (type)~%{~%~{case ~A : return ~A;~%~}default : assert(0); return 0;~%}~%}~%~%"
	    (mappend #'(lambda (type)
			 (let ((c-type (type-c-type type)))
			   (list (type-c-define type)
				 (if (null c-type)
				     "0"
				     (format nil "\"~A\"" c-type)))))
		     *types*))
    (format out "#define MAKE_CONST_PRIMARY_FUNCS \\~%~{MAKE_CONST_PRIMARY(~A, ~A, ~A)~^ \\~%~}~%~%"
	    (mappend #'(lambda (type)
			 (if (null (type-c-type type))
			     nil
			     (list (dcs (type-name type)) (type-c-type type) (type-c-define type))))
		     *types*))
    (format out "#define MAKE_CONST_COMPARATOR \\~%~{case ~A : return ~{prim1->v.~A_const~A == prim2->v.~2:*~A_const~A~^ && ~};~^ \\~%~}~%~%"
	    (mappend #'(lambda (type)
			 (if (null (type-c-type type))
			     nil
			   (list (type-c-define type)
				 (let ((dcsname (dcs (type-name type))))
				   (if (null (type-elements type))
				       (list dcsname "")
				     (mappend #'(lambda (element)
						  (list dcsname (string-concat "." element)))
					      (type-elements type)))))))
		     *types*))
    (labels ((printer (name spec-accessor)
	       (format out "#define ~A \\~%~{case ~A : fprintf_c(out, \"~A\"~{, ~A~}); break;~^ \\~%~}~%~%"
		       name
		       (mappend #'(lambda (type)
				    (let ((print-info (type-print-info type)))
				      (list (type-c-define type)
					    (funcall spec-accessor print-info)
					    (mapcar #'(lambda (arg-spec)
							(format nil arg-spec
								(format nil "primary->v.~A_const" (dcs (type-name type)))))
						    (car (last print-info))))))
				*types*))))
      (printer "TYPE_DEBUG_PRINTER" #'first)
      (printer "TYPE_C_PRINTER" #'second))))

(defun make-ops-file ()
  (with-open-file (out "opdefs.h" :direction :output :if-exists :supersede)
    (let ((*standard-output* out))
      (format t "~A~%#define NUM_OPS ~A~%~%" (make-op-defines) (length *operators*))
      (format t "static void~%init_ops (void)~%{~%~A}~%~%" (make-init-ops))
      (format t "static primary_t~%fold_rhs (rhs_t *rhs)~%{~%assert(rhs_is_foldable(rhs));~%switch(rhs->v.op.op->index)~%{~%~Adefault : assert(0);~%}~%}~%" (make-op-folders)))))