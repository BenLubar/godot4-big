## This file is ported from src/math/big/gcd_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2012 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

## This file implements a GCD benchmark.
## Usage: go test math/big -test.bench GCD

class_name GCDTest
extends TestSuite

## randInt returns a pseudo-random Int in the range [1<<(size-1), (1<<size) - 1]
func randInt(r: RandomNumberGenerator, size: int) -> BigInt:
	var n := BigInt.NewInt(1)
	n.Lsh(n, size-1)
	var x := BigInt.new()
	x.Rand(r.randi, n)
	x.Add(x, n) # make sure result > 1<<(size-1)
	return x

func runGCD(b0: TestingB, aSize: int, bSize: int) -> void:
	b0.Run("WithoutXY", func(b: TestingB) -> void:
		runGCDExt(b, aSize, bSize, false))
	b0.Run("WithXY", func(b: TestingB) -> void:
		runGCDExt(b, aSize, bSize, true))

func runGCDExt(b: TestingB, aSize: int, bSize: int, calcXY: bool) -> void:
	b.StopTimer()
	var r := RandomNumberGenerator.new()
	r.seed = 1234
	var aa := randInt(r, aSize)
	var bb := randInt(r, bSize)
	var x: BigInt
	var y: BigInt
	if calcXY:
		x = BigInt.new()
		y = BigInt.new()
	b.StartTimer()
	for i in b.N:
		BigInt.new().GCD(x, y, aa, bb)

func BenchmarkGCD10x10(b: TestingB) -> void:
	runGCD(b, 10, 10)
func BenchmarkGCD10x100(b: TestingB) -> void:
	runGCD(b, 10, 100)
func BenchmarkGCD10x1000(b: TestingB) -> void:
	runGCD(b, 10, 1000)
func BenchmarkGCD10x10000(b: TestingB) -> void:
	runGCD(b, 10, 10000)
func BenchmarkGCD10x100000(b: TestingB) -> void:
	runGCD(b, 10, 100000)
func BenchmarkGCD100x100(b: TestingB) -> void:
	runGCD(b, 100, 100)
func BenchmarkGCD100x1000(b: TestingB) -> void:
	runGCD(b, 100, 1000)
func BenchmarkGCD100x10000(b: TestingB) -> void:
	runGCD(b, 100, 10000)
func BenchmarkGCD100x100000(b: TestingB) -> void:
	runGCD(b, 100, 100000)
func BenchmarkGCD1000x1000(b: TestingB) -> void:
	runGCD(b, 1000, 1000)
func BenchmarkGCD1000x10000(b: TestingB) -> void:
	runGCD(b, 1000, 10000)
func BenchmarkGCD1000x100000(b: TestingB) -> void:
	runGCD(b, 1000, 100000)
func BenchmarkGCD10000x10000(b: TestingB) -> void:
	runGCD(b, 10000, 10000)
func BenchmarkGCD10000x100000(b: TestingB) -> void:
	runGCD(b, 10000, 100000)
func BenchmarkGCD100000x100000(b: TestingB) -> void:
	runGCD(b, 100000, 100000)
