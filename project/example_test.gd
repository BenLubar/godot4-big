## This file is ported from src/math/big/example_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2012 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

class_name ExampleTest
extends TestSuite

func ExampleRat_SetString(e: TestingE) -> void:
	var r := BigRat.new()
	r.SetString("355/113")
	e.Output(r.FloatString(3))

const ExampleRat_SetString_expected_output: PackedStringArray = [
	"3.142",
]

func ExampleInt_SetString(e: TestingE) -> void:
	var i := BigInt.new()
	i.SetString("644", 8) ## octal
	e.Output(i)

const ExampleInt_SetString_expected_output: PackedStringArray = [
	"420",
]

func ExampleFloat_SetString(e: TestingE) -> void:
	var f := BigFloat.new()
	f.SetString("3.14159")
	e.Output(f)

const ExampleFloat_SetString_expected_output: PackedStringArray = [
	"3.14159",
]

## This example demonstrates how to use big.Int to compute the smallest
## Fibonacci number with 100 decimal digits and to test whether it is prime.
func Example_fibonacci(e: TestingE) -> void:
	## Initialize two big ints with the first two numbers in the sequence.
	var a := BigInt.NewInt(0)
	var b := BigInt.NewInt(1)

	## Initialize limit as 10^99, the smallest integer with 100 digits.
	var limit := BigInt.new()
	limit.Exp(BigInt.NewInt(10), BigInt.NewInt(99))

	## Loop while a is smaller than 1e100.
	while a.Cmp(limit) < 0:
		## Compute the next Fibonacci number, storing it in a.
		a.Add(a, b)
		## Swap a and b so that b is the next number in the sequence.
		var temp := a
		a = b
		b = temp
	e.Output(a)

	## Test a for primality.
	## (ProbablyPrimes' argument sets the number of Miller-Rabin
	## rounds to be performed. 20 is a good value.)
	e.Output(a.ProbablyPrime(20))

const Example_fibonacci_expected_output: PackedStringArray = [
	"1344719667586153181419716641724567886890850696275767987106294472017884974410332069524504824747437757",
	"false",
]

## This example shows how to use big.Float to compute the square root of 2 with
## a precision of 200 bits, and how to print the result as a decimal number.
func Example_sqrt2(e: TestingE) -> void:
	## We'll do computations with 200 bits of precision in the mantissa.
	const prec := 200

	## Compute the square root of 2 using Newton's Method. We start with
	## an initial estimate for sqrt(2), and then iterate:
	##     x_{n+1} = 1/2 * ( x_n + (2.0 / x_n) )

	## Since Newton's Method doubles the number of correct digits at each
	## iteration, we need at least log_2(prec) steps.
	var steps := int(log(prec) / log(2)) + 1

	## Initialize values we need for the computation.
	var two := BigFloat.new()
	two.SetPrec(prec)
	two.SetInt64(2)
	var half := BigFloat.new()
	half.SetPrec(prec)
	half.SetFloat64(0.5)

	## Use 1 as the initial estimate.
	var x := BigFloat.new()
	x.SetPrec(prec)
	x.SetInt64(1)

	## We use t as a temporary variable. There's no need to set its precision
	## since big.Float values with unset (== 0) precision automatically assume
	## the largest precision of the arguments when used as the result (receiver)
	## of a big.Float operation.
	var t := BigFloat.new()

	## Iterate.
	for i in steps:
		t.Quo(two, x)  ## t = 2.0 / x_n
		t.Add(x, t)    ## t = x_n + (2.0 / x_n)
		x.Mul(half, t) ## x_{n+1} = 0.5 * t

	## We can use the usual fmt.Printf verbs since big.Float implements fmt.Formatter
	e.Output("sqrt(2) = %s" % [x.String(BigFloat.FORMAT_PLAIN, 50)])

	## Print the error between 2 and x*x.
	t.Mul(x, x) ## t = x*x
	t.Sub(two, t)
	e.Output("error = %s" % [t.String(BigFloat.FORMAT_SCIENTIFIC)])

const Example_sqrt2_expected_output: PackedStringArray = [
	"sqrt(2) = 1.41421356237309504880168872420969807856967187537695",
	"error = 0.000000e+00",
]
