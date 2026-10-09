## This file is ported from src/math/big/hilbert_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2009 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

## A little test program and benchmark for rational arithmetics.
## Computes a Hilbert matrix, its inverse, multiplies them
## and verifies that the product is the identity matrix.

class_name HilbertTest
extends TestSuite

class Matrix:
	var n: int
	var m: int
	var a: Array[BigRat]

	func _init(cols: int, rows: int) -> void:
		assert(0 <= cols)
		assert(0 <= rows)
		n = cols
		m = rows
		a.resize(cols * rows)

	func at(i: int, j: int) -> BigRat:
		assert(0 <= i and i < n)
		assert(0 <= j and j < m)
		return a[(i * m) + j]

	func setItem(i: int, j: int, x: BigRat) -> void:
		assert(0 <= i and i < n)
		assert(0 <= j and j < m)
		a[(i * m) + j] = x

	static func newUnit(size: int) -> Matrix:
		var unit := Matrix.new(size, size)
		for i in size:
			for j in size:
				var x := BigRat.NewRat(0, 1)
				if i == j:
					x.SetInt64(1)
				unit.setItem(i, j, x)
		return unit

	static func newHilbert(size: int) -> Matrix:
		var hilbert := Matrix.new(size, size)
		for i in size:
			for j in size:
				hilbert.setItem(i, j, BigRat.NewRat(1, i + j + 1))
		return hilbert

	static func newInverseHilbert(size: int) -> Matrix:
		var invhilbert := Matrix.new(size, size)
		for i in size:
			for j in size:
				var x1 := BigRat.new()
				var x2 := BigRat.new()
				var x3 := BigRat.new()
				var x4 := BigRat.new()
				var xi := BigInt.new()

				x1.SetInt64(i + j + 1)
				xi.Binomial(size + i, size - j - 1)
				x2.SetInt(xi)
				xi.Binomial(size + j, size - i - 1)
				x3.SetInt(xi)
				xi.Binomial(i + j, i)
				x4.SetInt(xi)

				x1.Mul(x1, x2)
				x1.Mul(x1, x3)
				x1.Mul(x1, x4)
				x1.Mul(x1, x4)

				if ((i + j) & 1) != 0:
					x1.Neg(x1)

				invhilbert.setItem(i, j, x1)

		return invhilbert

	func mul(other: Matrix) -> Matrix:
		assert(self.m == other.n)

		var product := Matrix.new(self.n, other.m)
		for i in product.n:
			for j in product.m:
				var x := BigRat.new()
				var y := BigRat.new()
				for k in self.m:
					y.Mul(self.at(i, k), other.at(k, j))
					x.Add(x, y)
				product.setItem(i, j, x)
		return product

	func eql(other: Matrix) -> bool:
		if self.n != other.n or self.m != other.m:
			return false
		for i in self.n:
			for j in self.m:
				if self.at(i, j).Cmp(other.at(i, j)) != 0:
					return false
		return true

	func _to_string() -> String:
		var s := ""
		for i in n:
			for j in m:
				s += "\t%s" % [at(i, j)]
			s += "\n"
		return s

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
