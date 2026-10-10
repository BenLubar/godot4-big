## This file is ported from src/math/big/arith_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2009 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

## This file defines tests of consistent behavior between assembly and Go versions of basic operators,
## as well as tests of pure Go implementations.

class_name ArithTest
extends TestSuite

static func u64ge(x: int, y: int) -> bool:
	var x1 := (x >> 32) & UINT32_MAX
	var x0 := x & UINT32_MAX
	var y1 := (y >> 32) & UINT32_MAX
	var y0 := y & UINT32_MAX
	return x1 > y1 or (x1 == y1 and x0 >= y0)

const words4: PackedInt64Array = [0, 1, -2, -1]
const words2: PackedInt64Array = [0, -1]
@warning_ignore("integer_division")
const muls: PackedInt64Array = [0, 1, 2, 3, 4, 5, INT64_MAX / 2, INT64_MAX, -4, -3, -2, -1]
const adds: PackedInt64Array = [0, 1, -2, -1]
@warning_ignore("integer_division")
const shifts: PackedInt64Array = [1, 2, 3, (64 / 4) - 1, 64 / 4, (64 / 4) + 1, (64 / 2) - 1, 64 / 2, (64 / 2) + 1, 64 - 3, 64 - 2, 64 - 1]

func TestAddVV(t: TestingT) -> void:
	testVV(t, "addVV", BigInt._addVV, BigInt._addVV_g)
func TestSubVV(t: TestingT) -> void:
	testVV(t, "subVV", BigInt._subVV, BigInt._subVV_g)
func TestAddVW(t: TestingT) -> void:
	testVW(t, "addVW", BigInt._addVW, BigInt._addVW_ref, words4)
func TestSubVW(t: TestingT) -> void:
	testVW(t, "subVW", BigInt._subVW, BigInt._subVW_ref, words4)
func TestLshVU(t: TestingT) -> void:
	testVU(t, "lshVU", BigInt._lshVU, BigInt._lshVU_g, shifts)
func TestRshVU(t: TestingT) -> void:
	testVU(t, "rshVU", BigInt._rshVU, BigInt._rshVU_g, shifts)
func TestMulAddVWW(t: TestingT) -> void:
	testVWW(t, "mulAddVWW", BigInt._mulAddVWW, BigInt._mulAddVWW_g, muls)
func TestAddMulVVWW(t: TestingT) -> void:
	testVVWW(t, "addMulVVWW", BigInt._addMulVVWW, BigInt._addMulVVWW_g, muls, adds)

## Note: It would be nice to avoid all the duplication of these test variants,
## but the only obvious way is to use reflection. These tests are already
## pretty expensive, and hitting them with reflect call overhead would
## reduce the amount of exhaustive testing it's reasonable to do, so instead
## we put up with the duplication.

static func testVV(t: TestingT, name: String, fn: Callable, ref: Callable) -> void:
	t.Error("TODO")
#	for size := range 100 {
#		xx := make([]Word, 1+size+1)
#		yy := make([]Word, 1+size+1)
#		zz := make([]Word, 1+size+1)
#		words := words4
#		if size > 5 {
#			words = words2
#		}
#		if size > 10 {
#			words = nil // random
#		}
#		for x := range nats(words, size) {
#			for y := range nats(words, size) {
#				wantZ := make([]Word, size)
#				wantC := ref(wantZ, x, y)
#
#				for _, inplace := range []bool{false, true} {
#					name := name
#					if inplace {
#						name = "in-place " + name
#					}
#					setSlice(xx, 1, x)
#					setSlice(yy, 2, y)
#					zz := zz
#					if inplace {
#						zz = xx
#					} else {
#						for i := range zz {
#							zz[i] = 0x9876
#						}
#					}
#					setSlice(zz, 3, nil)
#					c := fn(zz[1:1+size], xx[1:1+size], yy[1:1+size])
#					if !slices.Equal(zz[1:1+size], wantZ) || c != wantC {
#						t.Errorf("%s(%#x, %#x) = %#x, %#x, want %#x, %#x", name, x, y, zz[1:1+size], c, wantZ, wantC)
#					}
#					if !inplace {
#						checkSlice(t, name, "x", xx, 1, x)
#					}
#					checkSlice(t, name, "y", yy, 2, y)
#					checkSlice(t, name, "z", zz, 3, nil)
#					if t.Failed() {
#						t.FailNow()
#					}
#				}
#			}
#		}
#	}
#}

static func testVW(t: TestingT, name: String, fn: Callable, ref: Callable, ws: PackedInt64Array) -> void:
	t.Error("TODO")
#	const (
#		magic0 = 0x123450
#		magic1 = 0x543210
#	)
#
#	for size := range 100 {
#		xx := make([]Word, 1+size+1)
#		zz := make([]Word, 1+size+1)
#		words := words4
#		if size > 5 {
#			words = words2
#		}
#		if size > 10 {
#			words = nil // random
#		}
#		for x := range nats(words, size) {
#			for _, w := range ws {
#				wantZ := make([]Word, size)
#				wantC := ref(wantZ, x, w)
#
#				copy(xx[1:], x)
#				for _, inplace := range []bool{false, true} {
#					name := name
#					if inplace {
#						name = "in-place " + name
#					}
#					setSlice(xx, 1, x)
#					zz := zz
#					if inplace {
#						zz = xx
#					} else {
#						for i := range zz {
#							zz[i] = 0x9876
#						}
#					}
#					setSlice(zz, 2, nil)
#					c := fn(zz[1:1+size], xx[1:1+size], w)
#					if !slices.Equal(zz[1:1+size], wantZ) || c != wantC {
#						t.Errorf("%s(%#x, %#x) = %#x, %#x, want %#x, %#x", name, x, w, zz[1:1+size], c, wantZ, wantC)
#					}
#					if !inplace {
#						checkSlice(t, name, "x", xx, 1, x)
#					}
#					checkSlice(t, name, "z", zz, 2, nil)
#					if t.Failed() {
#						t.FailNow()
#					}
#				}
#			}
#		}
#	}
#}

static func testVU(t: TestingT, name: String, fn: Callable, ref: Callable, ys: PackedInt64Array) -> void:
	t.Error("TODO")
#	wys := make([]Word, len(ys))
#	for i, y := range ys {
#		wys[i] = Word(y)
#	}
#	testVW(t, name,
#		func(z, x []Word, y Word) Word { return fn(z, x, uint(y)) },
#		func(z, x []Word, y Word) Word { return ref(z, x, uint(y)) },
#		wys)
#}

static func testVWW(t: TestingT, name: String, fn: Callable, ref: Callable, ys: PackedInt64Array) -> void:
	t.Error("TODO")
#	const (
#		magic0 = 0x123450
#		magic1 = 0x543210
#	)
#
#	for size := range 100 {
#		xx := make([]Word, 1+size+1)
#		zz := make([]Word, 1+size+1)
#		words := words4
#		if size > 5 {
#			words = words2
#		}
#		if size > 10 {
#			words = nil // random
#		}
#		for x := range nats(words, size) {
#			for _, y := range ys {
#				for _, r := range ys {
#					wantZ := make([]Word, size)
#					wantC := ref(wantZ, x, y, r)
#
#					copy(xx[1:], x)
#					for _, inplace := range []bool{false, true} {
#						name := name
#						if inplace {
#							name = "in-place " + name
#						}
#						setSlice(xx, 1, x)
#						zz := zz
#						if inplace {
#							zz = xx
#						} else {
#							for i := range zz {
#								zz[i] = 0x9876
#							}
#						}
#						setSlice(zz, 2, nil)
#						c := fn(zz[1:1+size], xx[1:1+size], y, r)
#						if !slices.Equal(zz[1:1+size], wantZ) || c != wantC {
#							t.Errorf("%s(%#x, %#x, %#x) = %#x, %#x, want %#x, %#x", name, x, y, r, zz[1:1+size], c, wantZ, wantC)
#						}
#						if !inplace {
#							checkSlice(t, name, "x", xx, 1, x)
#						}
#						checkSlice(t, name, "z", zz, 2, nil)
#						if t.Failed() {
#							t.FailNow()
#						}
#					}
#				}
#			}
#		}
#	}
#}

static func testVVU(t: TestingT, name: String, fn: Callable, ref: Callable, shifts: PackedInt64Array) -> void:
	t.Error("TODO")
#	for size := range 100 {
#		xx := make([]Word, 1+size+1)
#		yy := make([]Word, 1+size+1)
#		zz := make([]Word, 1+size+1)
#		words := words4
#		if size > 5 {
#			words = words2
#		}
#		if size > 10 {
#			words = nil // random
#		}
#		for x := range nats(words, size) {
#			for y := range nats(words, size) {
#				for _, s := range shifts {
#					wantZ := make([]Word, size)
#					wantC := ref(wantZ, x, y, s)
#
#					for _, inplace := range []bool{false, true} {
#						name := name
#						if inplace {
#							name = "in-place " + name
#						}
#						setSlice(xx, 1, x)
#						setSlice(yy, 2, y)
#						zz := zz
#						if inplace {
#							zz = xx
#						} else {
#							for i := range zz {
#								zz[i] = 0x9876
#							}
#						}
#						setSlice(zz, 3, nil)
#						c := fn(zz[1:1+size], xx[1:1+size], yy[1:1+size], s)
#						if !slices.Equal(zz[1:1+size], wantZ) || c != wantC {
#							t.Errorf("%s(%#x, %#x, %#x) = %#x, %#x, want %#x, %#x", name, x, y, s, zz[1:1+size], c, wantZ, wantC)
#						}
#						if !inplace {
#							checkSlice(t, name, "x", xx, 1, x)
#						}
#						checkSlice(t, name, "y", yy, 2, y)
#						checkSlice(t, name, "z", zz, 3, nil)
#						if t.Failed() {
#							t.FailNow()
#						}
#					}
#				}
#			}
#		}
#	}
#}

static func testVVWW(t: TestingT, name: String, fn: Callable, ref: Callable, ms: PackedInt64Array, as_: PackedInt64Array) -> void:
	t.Error("TODO")
#	for size := range 100 {
#		zz := make([]Word, 1+size+1)
#		xx := make([]Word, 1+size+1)
#		yy := make([]Word, 1+size+1)
#		words := words4
#		if size > 3 {
#			words = words2
#		}
#		if size > 7 {
#			words = nil // random
#		}
#		for x := range nats(words, size) {
#			for y := range nats(words, size) {
#				for _, m := range ms {
#					for _, a := range as {
#						wantZ := make([]Word, size)
#						wantC := ref(wantZ, x, y, m, a)
#
#						for _, inplace := range []bool{false, true} {
#							name := name
#							if inplace {
#								name = "in-place " + name
#							}
#							setSlice(xx, 1, x)
#							setSlice(yy, 2, y)
#							zz := zz
#							if inplace {
#								zz = xx
#							} else {
#								for i := range zz {
#									zz[i] = 0x9876
#								}
#							}
#							setSlice(zz, 3, nil)
#							c := fn(zz[1:1+size], xx[1:1+size], yy[1:1+size], m, a)
#							if !slices.Equal(zz[1:1+size], wantZ) || c != wantC {
#								t.Errorf("%s(%#x, %#x, %#x, %#x) = %#x, %#x, want %#x, %#x", name, x, y, m, a, zz[1:1+size], c, wantZ, wantC)
#							}
#							if !inplace {
#								checkSlice(t, name, "x", xx, 1, x)
#							}
#							checkSlice(t, name, "y", yy, 2, y)
#							checkSlice(t, name, "z", zz, 3, nil)
#							if t.Failed() {
#								t.FailNow()
#							}
#						}
#					}
#				}
#			}
#		}
#	}
#}

const magic0 := 0x123450
const magic1 := 0x543210

## setSlice sets x[1:len(x)-1] to orig, leaving magic values in x[0] and x[len(x)-1]
## so that we can tell if routines accidentally write before or after the data.
static func setSlice(x: PackedInt64Array, id: int, orig: PackedInt64Array) -> void:
	x[0] = magic0 + id
	for i in range(1, len(x) - 1):
		x[i] = orig[i - 1]
	x[-1] = magic1 + id

## checkSlice checks that the magic values left by setSlices are still there.
## If orig != nil, it also checks that the actual data in x is unmodified since setSlice.
static func checkSlice(t: TestingT, name: String, val: String, x: PackedInt64Array, id: int, orig: PackedInt64Array) -> void:
	if x[0] != magic0 + id:
		t.Error("%s smashed %s[-1]" % [name, val])
	if x[-1] != magic1 + id:
		t.Error("%s smashed %s[len(%s)]" % [name, val, val])
	if not orig.is_empty() and x.slice(1, len(x) - 1) != orig:
		t.Error("%s smashed %s: have %s, want %s" % [name, val, x.slice(1, len(x) - 1), orig])

## nats returns a sequence of interesting nats of the given size:
##
##   - all 0
##   - all ^0
##   - all possible combinations of words
##   - ten random values
static func nats(words: PackedInt64Array, size: int) -> Callable:
	return func(y: Callable) -> void:
		var w: PackedInt64Array
		w.resize(size)
		if size == 0:
			y.call(w)
			return

		## all 0
		w.fill(0)
		if not y.call(w):
			return

		## all ^0
		w.fill(-1)
		if not y.call(w):
			return

		## all possible combinations of words
		var generate: Callable
		generate = func(i: int) -> bool:
			if i >= size:
				return y.call(w)
			for x in words:
				w[i] = x
				if not generate.call(i + 1):
					return false
			return true
		if not generate.call(0):
			return

		## ten random values
		var r := rng()
		for i in 10:
			for j in size:
				w[i] = rndW(r)
			if not y.call(w):
				return

## Always the same seed for reproducible results.
static func rng() -> RandomNumberGenerator:
	var rand := RandomNumberGenerator.new()
	rand.seed = 1
	rand.state = 2
	return rand

static func rndW(r: RandomNumberGenerator) -> int:
	return r.randi() | (r.randi() << 32)

static func rndV(r: RandomNumberGenerator, n: int) -> PackedInt64Array:
	var v: PackedInt64Array
	v.resize(n)
	for i in n:
		v[i] = rndW(r)
	return v

## Construct a vector comprising the same word, usually '0' or 'maximum uint'
static func makeWordVec(e: int, n: int) -> PackedInt64Array:
	var v: PackedInt64Array
	v.resize(n)
	v.fill(e)
	return v

const arglshVUIn: PackedInt64Array = [1, 2, 4, 8, 16, 32, 64, 0, 0, 0]
const arglshVUr0: PackedInt64Array = [1, 2, 4, 8, 16, 32, 64]
const arglshVUr1: PackedInt64Array = [2, 4, 8, 16, 32, 64, 128]
const arglshVUrWm1: PackedInt64Array = [1 << (64 - 1), 0, 1, 2, 4, 8, 16]

#struct {
#	d  []Word // d is a Word slice, the input parameters x and z come from this array.
#	l  uint   // l is the length of the input parameters x and z.
#	xp uint   // xp is the starting position of the input parameter x, x := d[xp:xp+l].
#	zp uint   // zp is the starting position of the input parameter z, z := d[zp:zp+l].
#	s  uint   // s is the shift number.
#	r  []Word // r is the expected output result z.
#	c  Word   // c is the expected return value.
#	m  string // message.
#}
const arglshVU: Array[Array] = [
	## test cases for lshVU
	[[1, -1, -1, -1, -1, -1, -0x4000000000000000, 0], 7, 0, 0, 1, [2, -2, -1, -1, -1, -1, INT64_MIN + 1], 1, "complete overlap of lshVU"],
	[[1, -1, -1, -1, -1, -1, -0x4000000000000000, 0, 0, 0, 0], 7, 0, 3, 1, [2, -2, -1, -1, -1, -1, INT64_MIN + 1], 1, "partial overlap by half of lshVU"],
	[[1, -1, -1, -1, -1, -1, -0x4000000000000000, 0, 0, 0, 0, 0, 0, 0], 7, 0, 6, 1, [2, -2, -1, -1, -1, -1, INT64_MIN + 1], 1, "partial overlap by 1 Word of lshVU"],
	[[1, -1, -1, -1, -1, -1, -0x4000000000000000, 0, 0, 0, 0, 0, 0, 0, 0], 7, 0, 7, 1, [2, -2, -1, -1, -1, -1, INT64_MIN + 1], 1, "no overlap of lshVU"],
	## additional test cases with shift values of 1 and (_W-1)
	[arglshVUIn, 7, 0, 0, 1, arglshVUr1, 0, "complete overlap of lshVU and shift of 1"],
	[arglshVUIn, 7, 0, 0, 64 - 1, arglshVUrWm1, 32, "complete overlap of lshVU and shift of _W - 1"],
	[arglshVUIn, 7, 0, 1, 1, arglshVUr1, 0, "partial overlap by 6 Words of lshVU and shift of 1"],
	[arglshVUIn, 7, 0, 1, 64 - 1, arglshVUrWm1, 32, "partial overlap by 6 Words of lshVU and shift of _W - 1"],
	[arglshVUIn, 7, 0, 2, 1, arglshVUr1, 0, "partial overlap by 5 Words of lshVU and shift of 1"],
	[arglshVUIn, 7, 0, 2, 64 - 1, arglshVUrWm1, 32, "partial overlap by 5 Words of lshVU abd shift of _W - 1"],
	[arglshVUIn, 7, 0, 3, 1, arglshVUr1, 0, "partial overlap by 4 Words of lshVU and shift of 1"],
	[arglshVUIn, 7, 0, 3, 64 - 1, arglshVUrWm1, 32, "partial overlap by 4 Words of lshVU and shift of _W - 1"],
]

const argrshVUIn: PackedInt64Array = [0, 0, 0, 1, 2, 4, 8, 16, 32, 64]
const argrshVUr0: PackedInt64Array = [1, 2, 4, 8, 16, 32, 64]
const argrshVUr1: PackedInt64Array = [0, 1, 2, 4, 8, 16, 32]
const argrshVUrWm1: PackedInt64Array = [4, 8, 16, 32, 64, 128, 0]

#struct {
#	d  []Word // d is a Word slice, the input parameters x and z come from this array.
#	l  uint   // l is the length of the input parameters x and z.
#	xp uint   // xp is the starting position of the input parameter x, x := d[xp:xp+l].
#	zp uint   // zp is the starting position of the input parameter z, z := d[zp:zp+l].
#	s  uint   // s is the shift number.
#	r  []Word // r is the expected output result z.
#	c  Word   // c is the expected return value.
#	m  string // message.
#}
const argrshVU: Array[Array] = [
	## test cases for rshVU
	[[0, 3, -1, -1, -1, -1, -1, INT64_MIN], 7, 1, 1, 1, [INT64_MIN + 1, -1, -1, -1, -1, INT64_MAX, 1 << (64 - 2)], INT64_MIN, "complete overlap of rshVU"],
	[[0, 0, 0, 0, 3, -1, -1, -1, -1, -1, INT64_MIN], 7, 4, 1, 1, [INT64_MIN + 1, -1, -1, -1, -1, INT64_MAX, 1 << (64 - 2)], INT64_MIN, "partial overlap by half of rshVU"],
	[[0, 0, 0, 0, 0, 0, 0, 3, -1, -1, -1, -1, -1, INT64_MIN], 7, 7, 1, 1, [INT64_MIN + 1, -1, -1, -1, -1, INT64_MAX, 1 << (64 - 2)], INT64_MIN, "partial overlap by 1 Word of rshVU"],
	[[0, 0, 0, 0, 0, 0, 0, 0, 3, -1, -1, -1, -1, -1, INT64_MIN], 7, 8, 1, 1, [INT64_MIN + 1, -1, -1, -1, -1, INT64_MAX, 1 << (64 - 2)], INT64_MIN, "no overlap of rshVU"],
	## additional test cases with shift values of 0, 1 and (_W-1)
	[argrshVUIn, 7, 3, 3, 1, argrshVUr1, INT64_MIN, "complete overlap of rshVU and shift of 1"],
	[argrshVUIn, 7, 3, 3, 64 - 1, argrshVUrWm1, 2, "complete overlap of rshVU and shift of _W - 1"],
	[argrshVUIn, 7, 3, 2, 1, argrshVUr1, INT64_MIN, "partial overlap by 6 Words of rshVU and shift of 1"],
	[argrshVUIn, 7, 3, 2, 64 - 1, argrshVUrWm1, 2, "partial overlap by 6 Words of rshVU and shift of _W - 1"],
	[argrshVUIn, 7, 3, 1, 1, argrshVUr1, INT64_MIN, "partial overlap by 5 Words of rshVU and shift of 1"],
	[argrshVUIn, 7, 3, 1, 64 - 1, argrshVUrWm1, 2, "partial overlap by 5 Words of rshVU and shift of _W - 1"],
	[argrshVUIn, 7, 3, 0, 1, argrshVUr1, INT64_MIN, "partial overlap by 4 Words of rshVU and shift of 1"],
	[argrshVUIn, 7, 3, 0, 64 - 1, argrshVUrWm1, 2, "partial overlap by 4 Words of rshVU and shift of _W - 1"],
]

#func testShiftFunc(t *testing.T, f func(z, x []Word, s uint) Word, a argVU) {
#	// work on copy of a.d to preserve the original data.
#	b := make([]Word, len(a.d))
#	copy(b, a.d)
#	z := b[a.zp : a.zp+a.l]
#	x := b[a.xp : a.xp+a.l]
#	c := f(z, x, a.s)
#	for i, zi := range z {
#		if zi != a.r[i] {
#			t.Errorf("d := %v, %s (d[%d:%d], d[%d:%d], %d)\n\tgot z[%d] = %#x; want %#x", a.d, a.m, a.zp, a.zp+a.l, a.xp, a.xp+a.l, a.s, i, zi, a.r[i])
#			break
#		}
#	}
#	if c != a.c {
#		t.Errorf("d := %v, %s (d[%d:%d], d[%d:%d], %d)\n\tgot c = %#x; want %#x", a.d, a.m, a.zp, a.zp+a.l, a.xp, a.xp+a.l, a.s, c, a.c)
#	}
#}

func TestShiftOverlap(t: TestingT) -> void:
	t.Error("TODO")
#	for _, a := range arglshVU {
#		arg := a
#		testShiftFunc(t, lshVU, arg)
#	}
#
#	for _, a := range argrshVU {
#		arg := a
#		testShiftFunc(t, rshVU, arg)
#	}
#}

func TestIssue31084(t: TestingT) -> void:
	## compute 10^n via 5^n << n.
	const n := 165
	var p := BigInt.new()
	p.Exp(BigInt.NewInt(5), BigInt.NewInt(n))
	p.Lsh(p, n)
	var got := p.String(10)
	var want := "1" + "0".repeat(n)
	if got != want:
		t.Error("lsh(%s, %s)\n\tgot  %s\n\twant %s" % [p, n, got, want])

const issue42838Value := "159309191113245227702888039776771180559110455519261878607388585338616290151305816094308987472018268594098344692611135542392730712890625"

func TestIssue42838(t: TestingT) -> void:
	const s := 192
	var z := BigInt.new()
	z.SetString(issue42838Value)
	z.Lsh(z, s)
	var got := z.String(10)
	var want := "1" + "0".repeat(s)
	if got != want:
		t.Error("lsh(%s, %s)\n\tgot  %s\n\twant %s" % [z, s, got, want])

#struct {
#	z, x nat
#	y, r Word
#	c    Word
#}
const prodVWW: Array[Array] = [
	[[], [], 0, 0, 0],
	[[0], [0], 0, 0, 0],
	[[991], [0], 0, 991, 0],
	[[0], [-1], 0, 0, 0],
	[[991], [-1], 0, 991, 0],
	[[0], [0], -1, 0, 0],
	[[991], [0], -1, 991, 0],
	[[1], [1], 1, 0, 0],
	[[992], [1], 1, 991, 0],
	[[22793], [991], 23, 0, 0],
	[[22800], [991], 23, 7, 0],
	[[0, 0, 0, 22793], [0, 0, 0, 991], 23, 0, 0],
	[[7, 0, 0, 22793], [0, 0, 0, 991], 23, 7, 0],
	[[0, 0, 0, 0], [7893475, 7395495, 798547395, 68943], 0, 0, 0],
	[[991, 0, 0, 0], [7893475, 7395495, 798547395, 68943], 0, 991, 0],
	[[0, 0, 0, 0], [0, 0, 0, 0], 894375984, 0, 0],
	[[991, 0, 0, 0], [0, 0, 0, 0], 894375984, 991, 0],
	[[-2], [-1], 2, 0, 1],
	[[-1], [-1], 2, 1, 1],
	[[-128], [-1], 128, 0, 127],
	[[-64], [-1], 128, 64, 127],
	[[-128, -1, -1, -1], [-1, -1, -1, -1], 128, 0, 127],
	[[-64, -1, -1, -1], [-1, -1, -1, -1], 128, 64, 127],
]

static func testFunVWW(t: TestingT, msg: String, f: Callable, a: Array) -> void:
	var z := BigInt.new()
	var zabs: PackedInt64Array
	zabs.resize(len(a[0]))
	z._abs = zabs
	var x := BigInt.new()
	x._abs = a[1]

	var c: int = f.call(z, x, a[2], a[3])

	if z._abs != PackedInt64Array(a[0]):
		t.Error("%s%s\n\tgot z = %s; want %s" % [msg, a, z._abs, a[0]])

	if c != a[4]:
		t.Error("%s%s\n\tgot c = %x; want %x" % [msg, a, c, a[4]])

## TODO(gri) mulAddVWW and divWVW are symmetric operations but
## their signature is not symmetric. Try to unify.

static func testFunWVW(t: TestingT, msg: String, f: Callable, a: Array) -> void:
	var z := BigInt.new()
	var zabs: PackedInt64Array
	zabs.resize(len(a[0]))
	z._abs = zabs
	var x := BigInt.new()
	x._abs = a[2]

	var r: int = f.call(z, a[1], x, a[3])
	if z._abs != PackedInt64Array(a[0]) or r != a[4]:
		t.Error("%s%s\nhave %s, %d\nwant %s, %d" % [msg, a, z, r, a[0], a[4]])

func TestFunVWW(t: TestingT) -> void:
	for a in prodVWW:
		var arg := a
		testFunVWW(t, "mulAddVWW_g", BigInt._mulAddVWW_g, arg)
		testFunVWW(t, "mulAddVWW", BigInt._mulAddVWW, arg)

		if a[2] != 0 and a[3] < a[2]:
			arg = [a[1], a[4], a[0], a[2], a[3]]
			testFunWVW(t, "divWVW", BigInt._divWVW, arg)

#struct {
#	x, y Word
#	q, r Word
#}
const mulWWTests: Array[Array] = [
	[-1, -1, -2, 1],
	## 32 bit only:
	#[0xc47dfa8c, 50911, 0x98a4, 0x998587f4],
]

func TestMulWW(t: TestingT) -> void:
	for i in len(mulWWTests):
		var test := mulWWTests[i]
		var qr := BigInt._bits_Mul(test[0], test[1])
		if qr[0] != test[2] or qr[1] != test[3]:
			t.Error("#%d got (%x, %x) want (%x, %x)" % [i, qr[0], qr[1], test[2], test[3]])

#struct {
#	x, y, c Word
#	q, r    Word
#}
const mulAddWWWTests: Array[Array] = [
	## TODO(agl): These will only work on 64-bit platforms.
	#[15064310297182388543, 0xe7df04d2d35d5d80, 13537600649892366549, 13644450054494335067, 10832252001440893781],
	#[15064310297182388543, 0xdab2f18048baa68d, 13644450054494335067, 12869334219691522700, 14233854684711418382],
	[-1, -1, 0, -2, 1],
	[-1, -1, -1, -1, 0],
]

func TestMulAddWWW(t: TestingT) -> void:
	for i in len(mulAddWWWTests):
		var test := mulAddWWWTests[i]
		var qr := BigInt._mulAddWWW(test[0], test[1], test[2])
		if qr[0] != test[3] or qr[1] != test[4]:
			t.Error("#%d got (%x, %x) want (%x, %x)" % [i, qr[0], qr[1], test[3], test[4]])

#struct {
#	x1, x0, y Word
#	q, r      Word
#}
const divWWTests: Array[Array] = [
	[INT64_MAX, 0, -1, INT64_MAX, INT64_MAX],
	[-0x4000000000000001, -1, -0x4000000000000000, -1, -0x4000000000000001],
]

const testsNumber := 1 << 16

func TestDivWW(t: TestingT) -> void:
	for i in len(divWWTests):
		var test := divWWTests[i]
		var rec := BigInt._reciprocalWord(test[2])
		var qr := BigInt._divWW(test[0], test[1], test[2], rec)
		if qr[0] != test[3] or qr[1] != test[4]:
			t.Error("#%d got (%x, %x) want (%x, %x)" % [i, qr[0], qr[1], test[3], test[4]])

	## random tests
	var r := rng()
	for i in testsNumber:
		var x1 := rndW(r)
		var x0 := rndW(r)
		var y := rndW(r)

		if u64ge(x1, y):
			continue

		var rec := BigInt._reciprocalWord(y)
		var qrGot := BigInt._divWW(x1, x0, y, rec)
		var qrWant := BigInt._bits_Div(x1, x0, y)
		if qrGot != qrWant:
			t.Error("#%d got (%x, %x) want (%x, %x)" % [i, qrGot[0], qrGot[1], qrWant[0], qrWant[1]])

## benchSizes are the benchmark word sizes.
const benchSizes: PackedInt64Array = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 32, 64, 100, 1000, 10000, 100000]

## bench runs benchmarks of fn for a variety of word sizes.
## It adds the given suffix (for example "/impl=go") to the benchmark names it creates,
## after a "/words=N" parameter. Putting words first makes it easier to run
## all benchmarks with a specific word size
## (go test -run=NONE '-bench=V/words=100$')
## even if different benchmarks have different numbers of other parameters.
static func bench(b0: TestingB, suffix: String, fn: Callable) -> void:
	for n in benchSizes:
		var r := rng()
		var z := BigInt.new()
		var x := BigInt.new()
		var y := BigInt.new()
		var zabs: PackedInt64Array
		zabs.resize(n)
		z._abs = zabs
		x._abs = rndV(r, n)
		y._abs = rndV(r, n)
		b0.Run("words=%d%s" % [n, suffix], func(b: TestingB) -> void:
			b.ResetTimer()
			b.SetBytes(n * 8)
			for i in b.N:
				fn.call(z, x, y))

## Benchmark basic I/O and arithmetic processing speed,
## to help estimate the upper bounds on other operations.

func BenchmarkCopyVV(b: TestingB) -> void:
	bench(b, "", benchVV(copyVV))

static func copyVV(z: BigInt, x: BigInt, _y: BigInt) -> int:
	z.Set(x)
	return 0

## Note: This benchmark consistently runs faster (even up to 2X faster on MB/s)
## with words=10 and words=100 than larger amounts like words=1000 or words=10000.
## The reason appears to that if you run 100-word addition loops repeatedly,
## they are independent calculations, and the processor speculates/pipelines/whatever
## to such a deep level that it can overlap the repeated loops.
## In contrast, if you run 1000-word or 10000-word loops repeatedly,
## the dependency chains are so long that the processor cannot overlap them.
## If we change arithVV to take the starting value of s and pass in the result
## from the previous arithVV, then even the 10-word or 100-loops become
## a single long dependency chain and the 2X disappears. But since we are
## using BenchmarkArithVV for a given word size to estimate the upper bound
## of, say, BenchmarkAddVV for that same word size, we actually want the
## dependency chain-length variation in BenchmarkArithVV too.
## It's just mysterious to see until you understand what is causing it.

func BenchmarkArithVV(b: TestingB) -> void:
	bench(b, "", benchVV(arithVV))

static func arithVV(z: BigInt, x: BigInt, y: BigInt) -> int:
	var a := 0
	var b := 0
	var c := 0
	var d := 0
	var e := 0
	var f := 0
	var g := 0
	var h := 0
	var i := 0
	var j := 0
	var zabs := z._abs
	if len(zabs) >= 8:
		a = 1
		b = 2
		c = 3
		d = 4
		e = 5
		f = 6
		g = 7
		h = 8
		i = 9
		j = 10
	var s := 0
	if len(zabs) < 10:
		## We don't really care about the speed here, but
		## do something so that the small word counts aren't all the same.
		for zi in zabs:
			s += zi
		return s
	@warning_ignore("integer_division")
	for k in len(zabs) / 10:
		s += a
		s += b
		s += c
		s += d
		s += e
		s += f
		s += g
		s += h
		s += i
		s += j
	return s

func BenchmarkAddVV(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVV(BigInt._addVV))
	bench(b, "/impl=go", benchVV(BigInt._addVV_g))

func BenchmarkSubVV(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVV(BigInt._subVV))
	bench(b, "/impl=go", benchVV(BigInt._subVV_g))

static func benchVV(fn: Callable) -> Callable:
	return func(z: BigInt, x: BigInt, y: BigInt) -> void:
		fn.call(z, x, y)

func BenchmarkAddVW(b: TestingB) -> void:
	bench(b, "/data=random", benchVW(BigInt._addVW, 123))
	bench(b, "/data=carry", benchCarryVW(BigInt._addVW, -1, 1))
	bench(b, "/data=shortcut", benchShortVW(BigInt._addVW, 123))

func BenchmarkSubVW(b: TestingB) -> void:
	bench(b, "/data=random", benchVW(BigInt._subVW, 123))
	bench(b, "/data=carry", benchCarryVW(BigInt._subVW, 0, 1))
	bench(b, "/data=shortcut", benchShortVW(BigInt._subVW, 123))

static func benchVW(fn: Callable, w: int) -> Callable:
	return func(z: BigInt, x: BigInt, _y: BigInt) -> void:
		fn.call(z, x, w)

static func benchCarryVW(fn: Callable, xi: int, w: int) -> Callable:
	return func(z: BigInt, x: BigInt, _y: BigInt) -> void:
		## Fill x with xi the first time we are called with a given x.
		## Otherwise x is random, so checking the first two elements is good enough.
		## Assume this is the warmup, so we don't need to worry about it taking longer.
		var xabs := x._abs
		if xabs[0] != w or (len(xabs) >= 2 and xabs[1] != w):
			xabs.fill(xi)
			x._abs = xabs
		fn.call(z, x, w)

static func benchShortVW(fn: Callable, w: int) -> Callable:
	## Note: calling fn with x not z, to benchmark in-place overwriting.
	return func(_z: BigInt, x: BigInt, _y: BigInt) -> void:
		fn.call(x, x, w)

func BenchmarkLshVU(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVU(BigInt._lshVU, 3))
	bench(b, "/impl=go", benchVU(BigInt._lshVU_g, 3))

func BenchmarkRshVU(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVU(BigInt._rshVU, 3))
	bench(b, "/impl=go", benchVU(BigInt._rshVU_g, 3))

static func benchVU(fn: Callable, s: int) -> Callable:
	return func(z: BigInt, x: BigInt, _y: BigInt) -> void:
		fn.call(z, x, s)

func BenchmarkMulAddVWW(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVWW(BigInt._mulAddVWW, 42, 100))
	bench(b, "/impl=go", benchVWW(BigInt._mulAddVWW_g, 42, 100))

static func benchVWW(fn: Callable, w1: int, w2: int) -> Callable:
	return func(z: BigInt, x: BigInt, _y: BigInt) -> void:
		fn.call(z, x, w1, w2)

func BenchmarkAddMulVVWW(b: TestingB) -> void:
	bench(b, "/impl=asm", benchVVWW(BigInt._addMulVVWW, 42, 100))
	bench(b, "/impl=go", benchVVWW(BigInt._addMulVVWW_g, 42, 100))

static func benchVVWW(fn: Callable, w1: int, w2: int) -> Callable:
	return func(z: BigInt, x: BigInt, y: BigInt) -> void:
		fn.call(z, x, y, w1, w2)

func BenchmarkDivWVW(b: TestingB) -> void:
	bench(b, "", benchWVW(BigInt._divWVW, 100, 200))

static func benchWVW(fn: Callable, w1: int, w2: int) -> Callable:
	return func(z: BigInt, x: BigInt, _y: BigInt) -> void:
		fn.call(z, w1, x, w2)
