# Copyright 2012 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name ExampleTest
extends TestSuite

func ExampleRat_SetString(e: TestingE) -> void:
	var r := BigRat.new()
	r.SetString("355/113")
	e.Output(r.FloatString(3))

	# Output:
	e.expected_output = [
		"3.142",
	]

func ExampleInt_SetString(e: TestingE) -> void:
	var i := BigInt.new()
	i.SetString("644", 8) # octal
	e.Output(i)

	# Output:
	e.expected_output = [
		"420",
	]

func ExampleFloat_SetString(e: TestingE) -> void:
	var f := BigFloat.new()
	f.SetString("3.14159")
	e.Output(f)

	# Output:
	e.expected_output = [
		"3.14159",
	]

func ExampleRat_Scan(e: TestingE) -> void:
	# The Scan function is rarely used directly;
	# the fmt package recognizes it as an implementation of fmt.Scanner.
	var r := BigRat.new()
	var err := r.SetString("1.5000")
	assert(err == OK)
	e.Output(r)

	# Output:
	e.expected_output = [
		"3/2",
	]

func ExampleInt_Scan(e: TestingE) -> void:
	# The Scan function is rarely used directly;
	# the fmt package recognizes it as an implementation of fmt.Scanner.
	var i := BigInt.new()
	var err := i.SetString("18446744073709551617")
	assert(err == OK)
	e.Output(i)

	# Output:
	e.expected_output = [
		"18446744073709551617",
	]

func ExampleFloat_Scan(e: TestingE) -> void:
	# The Scan function is rarely used directly;
	# the fmt package recognizes it as an implementation of fmt.Scanner.
	var f := BigFloat.new()
	var err := f.SetString("1.19282e99")
	assert(err == OK)
	e.Output(e)

	# Output:
	e.expected_output = [
		"1.19282e+99",
	]

# This example demonstrates how to use big.Int to compute the smallest
# Fibonacci number with 100 decimal digits and to test whether it is prime.
func Example_fibonacci(e: TestingE) -> void:
	# Initialize two big ints with the first two numbers in the sequence.
	var a := BigInt.NewInt(0)
	var b := BigInt.NewInt(1)

	# Initialize limit as 10^99, the smallest integer with 100 digits.
	var limit := BigInt.new()
	limit.Exp(BigInt.NewInt(10), BigInt.NewInt(99))

	# Loop while a is smaller than 1e100.
	while a.Cmp(limit) < 0:
		# Compute the next Fibonacci number, storing it in a.
		a.Add(a, b)
		# Swap a and b so that b is the next number in the sequence.
		var swap := a
		a = b
		b = swap
	e.Output(a) # 100-digit Fibonacci number

	# Test a for primality.
	# (ProbablyPrimes' argument sets the number of Miller-Rabin
	# rounds to be performed. 20 is a good value.)
	e.Output(a.ProbablyPrime(20))

	# Output:
	e.expected_output = [
		"1344719667586153181419716641724567886890850696275767987106294472017884974410332069524504824747437757",
		"false",
	]

# This example shows how to use big.Float to compute the square root of 2 with
# a precision of 200 bits, and how to print the result as a decimal number.
func Example_sqrt2(e: TestingE) -> void:
	# We'll do computations with 200 bits of precision in the mantissa.
	const prec := 200

	# Compute the square root of 2 using Newton's Method. We start with
	# an initial estimate for sqrt(2), and then iterate:
	#     x_{n+1} = 1/2 * ( x_n + (2.0 / x_n) )

	# Since Newton's Method doubles the number of correct digits at each
	# iteration, we need at least log_2(prec) steps.
	var steps := floori(log(prec) / log(2))

	# Initialize values we need for the computation.
	var two := BigFloat.new()
	two.SetPrec(prec)
	two.SetInt64(2)

	var half := BigFloat.new()
	half.SetPrec(prec)
	half.SetFloat64(0.5)

	# Use 1 as the initial estimate.
	var x := BigFloat.new()
	x.SetPrec(prec)
	x.SetInt64(1)

	# We use t as a temporary variable. There's no need to set its precision
	# since big.Float values with unset (== 0) precision automatically assume
	# the largest precision of the arguments when used as the result (receiver)
	# of a big.Float operation.
	var t := BigFloat.new()

	# Iterate.
	for i in steps + 1:
		t.Quo(two, x)  # t = 2.0 / x_n
		t.Add(x, t)    # t = x_n + (2.0 / x_n)
		x.Mul(half, t) # x_{n+1} = 0.5 * t

	e.Output("sqrt(2) = " + x.String(BigFloat.FORMAT_PLAIN, 50))

	# Print the error between 2 and x*x.
	t.Mul(x, x) # t = x*x
	t.Sub(two, t)
	e.Output("error = " + t.String(BigFloat.FORMAT_SCIENTIFIC))

	# Output:
	e.expected_output = [
		"sqrt(2) = 1.41421356237309504880168872420969807856967187537695",
		"error = 0.000000e+00",
	]
