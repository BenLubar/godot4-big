# Copyright 2015 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name FloatExampleTest
extends TestSuite

func ExampleFloat_Add(e: TestingE) -> void:
	# Operate on numbers of different precision.
	var x := BigFloat.new()
	var y := BigFloat.new()
	var z := BigFloat.new()

	x.SetInt64(1000)          # x is automatically set to 64bit precision
	y.SetFloat64(2.718281828) # y is automatically set to 53bit precision
	z.SetPrec(32)
	z.Add(x, y)

	e.Output("x = %s (%s, prec = %d, acc = %s)" % [x, x.String(BigFloat.FORMAT_HEX_NORMAL), x.Prec(), FloatTest.accuracyName[x.Acc()]])
	e.Output("y = %s (%s, prec = %d, acc = %s)" % [y, y.String(BigFloat.FORMAT_HEX_NORMAL), y.Prec(), FloatTest.accuracyName[y.Acc()]])
	e.Output("z = %s (%s, prec = %d, acc = %s)" % [z, z.String(BigFloat.FORMAT_HEX_NORMAL), z.Prec(), FloatTest.accuracyName[z.Acc()]])

	# Output:
	e.expected_output = [
		"x = 1000 (0x.fap+10, prec = 64, acc = Exact)",
		"y = 2.718281828 (0x.adf85458248cd8p+2, prec = 53, acc = Exact)",
		"z = 1002.718282 (0x.faadf854p+10, prec = 32, acc = Below)",
	]

func ExampleFloat_shift(e: TestingE) -> void:
	# Implement Float "shift" by modifying the (binary) exponents directly.
	for s in range(-5, 6):
		var x := BigFloat.NewFloat(0.5)
		x.SetMantExp(x, x.MantExp(null) + s) # shift x by s
		e.Output(x)

	# Output:
	e.expected_output = [
		"0.015625",
		"0.03125",
		"0.0625",
		"0.125",
		"0.25",
		"0.5",
		"1",
		"2",
		"4",
		"8",
		"16",
	]

func ExampleFloat_Cmp(e: TestingE) -> void:
	var inf := INF
	var zero := 0.0

	var operands: PackedFloat64Array = [-inf, -1.2, -zero, 0, +1.2, +inf]

	e.Output("   x     y  cmp")
	e.Output("---------------")
	for x64 in operands:
		var x := BigFloat.NewFloat(x64)
		for y64 in operands:
			var y := BigFloat.NewFloat(y64)
			e.Output("%4s  %4s  %3d" % [x, y, x.Cmp(y)])
		e.Output("")

	# Output:
	e.expected_output = [
		"   x     y  cmp",
		"---------------",
		"-Inf  -Inf    0",
		"-Inf  -1.2   -1",
		"-Inf    -0   -1",
		"-Inf     0   -1",
		"-Inf   1.2   -1",
		"-Inf  +Inf   -1",
		"",
		"-1.2  -Inf    1",
		"-1.2  -1.2    0",
		"-1.2    -0   -1",
		"-1.2     0   -1",
		"-1.2   1.2   -1",
		"-1.2  +Inf   -1",
		"",
		"  -0  -Inf    1",
		"  -0  -1.2    1",
		"  -0    -0    0",
		"  -0     0    0",
		"  -0   1.2   -1",
		"  -0  +Inf   -1",
		"",
		"   0  -Inf    1",
		"   0  -1.2    1",
		"   0    -0    0",
		"   0     0    0",
		"   0   1.2   -1",
		"   0  +Inf   -1",
		"",
		" 1.2  -Inf    1",
		" 1.2  -1.2    1",
		" 1.2    -0    1",
		" 1.2     0    1",
		" 1.2   1.2    0",
		" 1.2  +Inf   -1",
		"",
		"+Inf  -Inf    1",
		"+Inf  -1.2    1",
		"+Inf    -0    1",
		"+Inf     0    1",
		"+Inf   1.2    1",
		"+Inf  +Inf    0",
	]

const roundingModeName: Dictionary[BigFloat.RoundingMode, String] = {
	BigFloat.TO_NEAREST_EVEN: "ToNearestEven",
	BigFloat.TO_NEAREST_AWAY: "ToNearestAway",
	BigFloat.TO_ZERO: "ToZero",
	BigFloat.AWAY_FROM_ZERO: "AwayFromZero",
	BigFloat.TO_NEGATIVE_INF: "ToNegativeInf",
	BigFloat.TO_POSITIVE_INF: "ToPositiveInf",
}

func ExampleRoundingMode(e: TestingE) -> void:
	const operands: PackedFloat64Array = [2.6, 2.5, 2.1, -2.1, -2.5, -2.6]

	var line := "   x"
	for mode in range(BigFloat.TO_NEAREST_EVEN, BigFloat.TO_POSITIVE_INF + 1):
		line += "  " + roundingModeName[mode]
	e.Output(line)

	for f64 in operands:
		line = str(f64).lpad(4)
		for mode in range(BigFloat.TO_NEAREST_EVEN, BigFloat.TO_POSITIVE_INF + 1):
			# sample operands above require 2 bits to represent mantissa
			# set binary precision to 2 to round them to integer values
			var f := BigFloat.new()
			f.SetPrec(2)
			f.SetMode(mode)
			f.SetFloat64(f64)
			line += "  " + f.String().lpad(len(roundingModeName[mode]))
		e.Output(line)

	# Output:
	e.expected_output = [
		"   x  ToNearestEven  ToNearestAway  ToZero  AwayFromZero  ToNegativeInf  ToPositiveInf",
		" 2.6              3              3       2             3              2              3",
		" 2.5              2              3       2             3              2              3",
		" 2.1              2              2       2             3              2              3",
		"-2.1             -2             -2      -2            -3             -3             -2",
		"-2.5             -2             -3      -2            -3             -3             -2",
		"-2.6             -3             -3      -2            -3             -3             -2",
	]

func ExampleFloat_Copy(e: TestingE) -> void:
	var x := BigFloat.new()
	var z := BigFloat.new()

	x.SetFloat64(1.23)
	z.Copy(x)
	e.Output("a) z = %s, x = %s" % [z, x])

	z.SetInt64(42)
	e.Output("b) z = %s" % [z])

	x.SetPrec(1)
	z.Copy(x)
	e.Output("c) z = %s, x = %s, z == x = %s" % [z, x, z == x])

	# Output:
	e.expected_output = [
		"a) z = 1.23, x = 1.23",
		"b) z = 42",
		"c) z = 1, x = 1, z == x = false",
	]
