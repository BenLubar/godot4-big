# Copyright 2015 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name DecimalTest
extends TestSuite

func TestDecimalString(t: TestingT) -> void:
	for test in [
		["", 0, "0"],
		["", 1000, "0"], # exponent of 0 is ignored
		["12345", 0, "0.12345"],
		["12345", -3, "0.00012345"],
		["12345", +3, "123.45"],
		["12345", +10, "1234500000"],
	]:
		var got := BigFloat._test_decimal_raw_string((test[0] as String).to_ascii_buffer(), test[1])
		var want: String = test[2]
		if got != want:
			t.Error("%s e %d == %s; want %s" % [test[0], test[1], got, want])

func TestDecimalInit(t: TestingT) -> void:
	for test in [
		[0, 0, "0"],
		[0, -100, "0"],
		[0, 100, "0"],
		[1, 0, "1"],
		[1, 10, "1024"],
		[1, 100, "1267650600228229401496703205376"],
		[1, -100, "0.0000000000000000000000000000007888609052210118054117285652827862296732064351090230047702789306640625"],
		[12345678, 8, "3160493568"],
		[12345678, -8, "48225.3046875"],
		[195312, 9, "99999744"],
		[1953125, 9, "1000000000"],
	]:
		var got := BigFloat._test_decimal_string([test[0]], test[1])
		var want: String = test[2]
		if got != want:
			t.Error("%d << %d == %s; want %s" % [test[0], test[1], got, want])

func TestDecimalRounding(t: TestingT) -> void:
	for test in [
		[0, 0, "0", "0", "0"],
		[0, 1, "0", "0", "0"],

		[1, 0, "0", "0", "10"],
		[5, 0, "0", "0", "10"],
		[9, 0, "0", "10", "10"],

		[15, 1, "10", "20", "20"],
		[45, 1, "40", "40", "50"],
		[95, 1, "90", "100", "100"],

		[12344999, 4, "12340000", "12340000", "12350000"],
		[12345000, 4, "12340000", "12340000", "12350000"],
		[12345001, 4, "12340000", "12350000", "12350000"],
		[23454999, 4, "23450000", "23450000", "23460000"],
		[23455000, 4, "23450000", "23460000", "23460000"],
		[23455001, 4, "23450000", "23460000", "23460000"],

		[99994999, 4, "99990000", "99990000", "100000000"],
		[99995000, 4, "99990000", "100000000", "100000000"],
		[99999999, 4, "99990000", "100000000", "100000000"],

		[12994999, 4, "12990000", "12990000", "13000000"],
		[12995000, 4, "12990000", "13000000", "13000000"],
		[12999999, 4, "12990000", "13000000", "13000000"],
	]:
		var x: PackedInt64Array = [test[0]]

		var got := BigFloat._test_decimal_round_string(x, 0, test[1], -1)
		var want: String = test[2]
		if got != want:
			t.Error("roundDown(%d, %d) = %s; want %s" % [test[0], test[1], got, want])

		got = BigFloat._test_decimal_round_string(x, 0, test[1], 0)
		want = test[3]
		if got != want:
			t.Error("round(%d, %d) = %s; want %s" % [test[0], test[1], got, want])

		got = BigFloat._test_decimal_round_string(x, 0, test[1], +1)
		want = test[4]
		if got != want:
			t.Error("roundUp(%d, %d) = %s; want %s" % [test[0], test[1], got, want])

func BenchmarkDecimalConversion(b: TestingB) -> void:
	var natOne: PackedInt64Array = [1]
	for i in b.N:
		for shift in range(-100, 101):
			var _sink := BigFloat._test_decimal_string(natOne, shift)

func BenchmarkFloatString(b0: TestingB) -> void:
	var x := BigFloat.new()
	for prec in [100, 1000, 10000, 100000]:
		x.SetPrec(prec)
		x.SetRat(BigRat.NewRat(1, 3))
		b0.Run("%d" % [prec], func(b: TestingB) -> void:
			for i in b.N:
				var _sink := x.String())
