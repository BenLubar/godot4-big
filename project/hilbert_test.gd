# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# A little test program and benchmark for rational arithmetics.
# Computes a Hilbert matrix, its inverse, multiplies them
# and verifies that the product is the identity matrix.

class_name HilbertTest
extends TestSuite

class Matrix:
	var n: int
	var m: int
	var a: Array[BigRat]

	func item_at(i: int, j: int) -> BigRat:
		assert(0 <= i and i < n)
		assert(0 <= j and j < m)
		return a[(i * m) + j]

	func set_item(i: int, j: int, x: BigRat) -> void:
		assert(0 <= i and i < n)
		assert(0 <= j and j < m)
		a[(i * m) + j] = x

	func mul(b: Matrix) -> Matrix:
		assert(m == b.n)
		var c := newMatrix(n, b.m)
		var t := BigRat.new()
		for i in c.n:
			for j in c.m:
				var x := BigRat.NewRat(0, 1)
				for k in m:
					t.Mul(item_at(i, k), b.item_at(k, j))
					x.Add(x, t)
				c.set_item(i, j, x)
		return c

	func eql(b: Matrix) -> bool:
		if n != b.n or m != b.m:
			return false
		for i in n:
			for j in m:
				if item_at(i, j).Cmp(b.item_at(i, j)) != 0:
					return false
		return true

	func _to_string() -> String:
		var s := ""
		for i in n:
			s += "\n"
			for j in m:
				s += "\t%s" % [item_at(i, j)]
		return s

	static func newMatrix(cols: int, rows: int) -> Matrix:
		assert(cols > 0 and rows > 0)

		var matrix := Matrix.new()
		matrix.n = cols
		matrix.m = rows
		matrix.a.resize(cols * rows)

		return matrix

	static func newUnit(size: int) -> Matrix:
		var matrix := newMatrix(size, size)
		for i in size:
			for j in size:
				var x := BigRat.NewRat(0, 1)
				if i == j:
					x.SetInt64(1)
				matrix.set_item(i, j, x)
		return matrix

	static func newHilbert(size: int) -> Matrix:
		var matrix := newMatrix(size, size)
		for i in size:
			for j in size:
				matrix.set_item(i, j, BigRat.NewRat(1, i + j + 1))
		return matrix

	static func newInverseHilbert(size: int) -> Matrix:
		var matrix := newMatrix(size, size)
		for i in size:
			for j in size:
				var xt := BigInt.new()
				var x1 := BigRat.new()
				x1.SetInt64(i + j + 1)
				var x2 := BigRat.new()
				xt.Binomial(size + i, size - j - 1)
				x2.SetInt(xt)
				var x3 := BigRat.new()
				xt.Binomial(size + j, size - i - 1)
				x3.SetInt(xt)
				var x4 := BigRat.new()
				xt.Binomial(i + j, i)
				x4.SetInt(xt)

				x1.Mul(x1, x2)
				x1.Mul(x1, x3)
				x1.Mul(x1, x4)
				x1.Mul(x1, x4)

				if ((i + j) & 1) != 0:
					x1.Neg(x1)

				matrix.set_item(i, j, x1)
		return matrix

func doHilbert(t: TestingT, n: int) -> void:
	var a := Matrix.newHilbert(n)
	var b := Matrix.newInverseHilbert(n)
	var I := Matrix.newUnit(n)
	var ab := a.mul(b)
	if not ab.eql(I):
		t.Error("a   = %s" % [a])
		t.Error("b   = %s" % [b])
		t.Error("a*b = %s" % [ab])
		t.Error("I   = %s" % [I])

func TestHilbert(t: TestingT) -> void:
	doHilbert(t, 10)

func BenchmarkHilbert(b: TestingB) -> void:
	for i in b.N:
		doHilbert(null, 10)
