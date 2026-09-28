# Copyright 2015 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# This file implements the Bits type used for testing Float operations
# via an independent (albeit slower) representations for floating-point
# numbers.

class_name BitsTest
extends TestSuite

# A Bits value b represents a finite floating-point number x of the form
#
#	x = 2**b[0] + 2**b[1] + ... 2**b[len(b)-1]
#
# The order of slice elements is not significant. Negative elements may be
# used to form fractions. A Bits value is normalized if each b[i] occurs at
# most once. For instance Bits{0, 0, 1} is not normalized but represents the
# same floating-point number as Bits{2}, which is normalized. The zero (nil)
# value of Bits is a ready to use Bits value and represents the value 0.
class Bits:
	var b: PackedInt64Array

	func add(y: Bits) -> Bits:
		var s := Bits.new()
		s.b.append_array(b)
		s.b.append_array(y.b)
		return s

	func mul(y: Bits) -> Bits:
		var p := Bits.new()
		for i in b:
			for j in y.b:
				p.b.append(i + j)
		return p

	# norm returns the normalized bits for x: It removes multiple equal entries
	# by treating them as an addition (e.g., Bits{5, 5} => Bits{6}), and it sorts
	# the result list for reproducible results.
	func norm() -> Bits:
		var m: Dictionary[int, bool]
		for i in b:
			while m.get(i, false):
				m[i] = false
				i += 1
			m[i] = true

		var z := Bits.new()
		z.b = m.keys().filter(func(i: int) -> bool:
			return m[i])
		z.b.sort()

		return z

	# round returns the Float value corresponding to x after rounding x
	# to prec bits according to mode.
	func round(prec: int, mode: BigFloat.RoundingMode) -> BigFloat:
		var x := norm()

		# determine range
		var smallest := 0
		var biggest := 0
		if not x.b.is_empty():
			smallest = x.b[0]
			biggest = x.b[len(x.b) - 1]

		var prec0 := biggest + 1 - smallest
		if prec >= prec0:
			return x.Float()
		# prec < prec0

		# determine bit 0, rounding, and sticky bit, and result bits z
		var bit0 := 0
		var rbit := 0
		var sbit := 0
		var z := Bits.new()
		var r := biggest - prec
		for i in x.b:
			if i == r:
				rbit = 1
			elif i < r:
				sbit = 1
			else:
				# b > r
				if i == r + 1:
					bit0 = 1
				z.b.append(i)

		# round
		var f := z.Float() # rounded to zero
		assert(mode != BigFloat.TO_NEAREST_AWAY)
		if (mode == BigFloat.TO_NEAREST_EVEN and rbit == 1 and (sbit == 1 or (sbit == 0 and bit0 != 0))) or mode == BigFloat.AWAY_FROM_ZERO:
			# round away from zero
			f.SetMode(BigFloat.TO_ZERO)
			f.SetPrec(prec)
			var zz := Bits.new()
			zz.b = [r + 1]
			f.Add(f, zz.Float())
		return f

	# Float returns the *Float z of the smallest possible precision such that
	# z = sum(2**bits[i]), with i = range bits. If multiple bits[i] are equal,
	# they are added: Bits{0, 1, 0}.Float() == 2**0 + 2**1 + 2**0 = 4.
	func Float() -> BigFloat:
		# handle 0
		if b.is_empty():
			return BigFloat.new()
		# len(bits) > 0

		# determine lsb exponent
		var smallest: int = Array(b).min()

		# create bit pattern
		var x := BigInt.new()
		for i in b:
			var badj := i - smallest
			# propagate carry if necessary
			while x.Bit(badj) != 0:
				x.SetBit(x, badj, 0)
				badj += 1
			x.SetBit(x, badj, 1)

		# create corresponding float
		var z := BigFloat.new()
		z.SetInt(x) # normalized
		var e := z._exp + smallest
		assert(BigFloat.MIN_EXP <= e and e <= BigFloat.MAX_EXP)
		z._exp = e
		return z

func TestMulBits(t: TestingT) -> void:
	for test in [
		[[], [], []],
		[[0], [0], [0]],
		[[0], [1], [1]],
		[[1], [1, 2, 3], [2, 3, 4]],
		[[-1], [1], [0]],
		[[-10, -1, 0, 1, 10], [1, 2, 3], [-9, -8, -7, 0, 1, 2, 1, 2, 3, 2, 3, 4, 11, 12, 13]],
	]:
		var x := Bits.new()
		x.b = test[0]
		var y := Bits.new()
		y.b = test[1]
		var z := x.mul(y)
		var got := z.b
		var want: PackedInt64Array = test[2]
		if got != want:
			t.Error("%s * %s = %s; want %s" % [test[0], test[1], got, want])

func TestNormBits(t: TestingT) -> void:
	for test in [
		[[], []],
		[[0], [0]],
		[[0, 0], [1]],
		[[3, 1, 1], [2, 3]],
		[[10, 9, 8, 7, 6, 6], [11]],
	]:
		var x := Bits.new()
		x.b = test[0]
		var got := x.norm().b
		var want: PackedInt64Array = test[1]
		if got != want:
			t.Error("normBits(%s) = %s; want %s" % [test[0], got, want])

func TestFromBits(t: TestingT) -> void:
	for test in [
		# all different bit numbers
		[[], "0"],
		[[0], "0x.8p+1"],
		[[1], "0x.8p+2"],
		[[-1], "0x.8p+0"],
		[[63], "0x.8p+64"],
		[[33, -30], "0x.8000000000000001p+34"],
		[[255, 0], "0x.8000000000000000000000000000000000000000000000000000000000000001p+256"],

		# multiple equal bit numbers
		[[0, 0], "0x.8p+2"],
		[[0, 0, 0, 0], "0x.8p+3"],
		[[0, 1, 0], "0x.8p+3"],
		[[2, 1, 0, 3, 1], "0x.88p+5"],
	]:
		var bits := Bits.new()
		bits.b = test[0]
		var f := bits.Float()
		var got := f.String(BigFloat.FORMAT_HEX_NORMAL, 0)
		var want: String = test[1]
		if got != want:
			t.Error("setBits(%s) = %s; want %s" % [test[0], got, want])
