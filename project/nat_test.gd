# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name NatTest
extends TestSuite

const cmpTests: Array[Array] = [
	[[], [], 0],
	[[0], [0], 0],
	[[0], [1], -1],
	[[1], [0], 1],
	[[1], [1], 0],
	[[0, -1], [1], 1],
	[[1], [0, -1], -1],
	[[1, -1], [0, -1], 1],
	[[0, -1], [1, -1], -1],
	[[16, 571956, 8794, 68], [837, 9146, 1, 754489], -1],
	[[34986, 41, 105, 1957], [56, 7458, 104, 1957], 1],
]

func TestCmp(t: TestingT) -> void:
	t.Error("TODO")
#	for i, a := range cmpTests {
#		r := a.x.cmp(a.y)
#		if r != a.r {
#			t.Errorf("#%d got r = %v; want %v", i, r, a.r)
#		}
#	}

#type funNN func(z, x, y nat) nat
#type funSNN func(z nat, stk *stack, x, y nat) nat
#type argNN struct {
#	z, x, y nat
#}

const sumNN: Array[Array] = [
	[[], [], []],
	[[1], [], [1]],
	[[1111111110], [123456789], [987654321]],
	[[0, 0, 0, 1], [], [0, 0, 0, 1]],
	[[0, 0, 0, 1111111110], [0, 0, 0, 123456789], [0, 0, 0, 987654321]],
	[[0, 0, 0, 1], [0, 0, -1], [0, 0, 1]],
]

static var prodNN := prodTests()

static func permute(rand: RandomNumberGenerator, x: PackedInt64Array) -> void:
	for i in range(len(x) - 1, 0, -1):
		var j := rand.randi_range(0, i)
		var swap := x[i]
		x[i] = x[j]
		x[j] = swap

static func norm(x: PackedInt64Array) -> PackedInt64Array:
	var i := len(x)
	while i > 0 and x[i - 1] == 0:
		i -= 1
	return x.slice(0, i)

# testMul returns the product of x and y using the grade-school algorithm,
# as a reference implementation.
static func testMul(x: PackedInt64Array, y: PackedInt64Array) -> PackedInt64Array:
	var z: PackedInt64Array
	z.resize(len(x) + len(y))
	for i in len(x):
		var xi := x[i]
		for j in len(y):
			var yj := y[j]
			var hilo := BigInt._bits_Mul(xi, yj)
			var k := i + j
			var sc := BigInt._bits_Add(z[k], hilo[1], 0)
			z[k] = sc[0]
			k += 1
			while hilo[0] != 0 or sc[1] != 0:
				sc = BigInt._bits_Add(z[k], hilo[0], sc[1])
				hilo[0] = 0
				z[k] = sc[0]
				k += 1
	return norm(z)

static func prodTests() -> Array[Array]:
	var rand := RandomNumberGenerator.new()
	rand.seed = 0

	var tests: Array[Array]
	for size in 10:
		var x: PackedInt64Array
		var y: PackedInt64Array
		for i in size:
			x.append(i + 1)
			y.append(i + 1 + size)
		permute(rand, x)
		permute(rand, y)
		x = norm(x)
		y = norm(y)
		tests.append([testMul(x, y), x, y])

	const words: PackedInt64Array = [0, 1, 2, 3, 4, -1, -2, -3, -4]
	for size in 10:
		if size == 0:
			continue # already tested the only 0-length possibility above
		for j in 10:
			var x: PackedInt64Array
			var y: PackedInt64Array
			x.resize(size)
			y.resize(size)
			for i in size:
				x[i] = words[rand.randi_range(0, len(words) - 1)]
				y[i] = words[rand.randi_range(0, len(words) - 1)]
			x = norm(x)
			y = norm(y)
			tests.append([testMul(x, y), x, y])
	tests.append_array(prodNNExtra())
	return tests

static func prodNNExtra() -> Array[Array]:
	return [
		[[], [991], []],
		[[991], [991], [1]],
		[[991 * 991], [991], [991]],
		[[8, 22, 15], [2, 3], [4, 5]],
		[[10, 27, 52, 45, 28], [2, 3, 4], [5, 6, 7]],
		[[12, 32, 61, 100, 94, 76, 45], [2, 3, 4, 5], [6, 7, 8, 9]],
		[[14, 37, 70, 114, 170, 166, 148, 115, 66], [2, 3, 4, 5, 6], [7, 8, 9, 10, 11]],
		[[991 * 991, 991 * 2, 1], [991, 1], [991, 1]],
		[[991 * 991, 991 * 777 * 2, 777 * 777], [991, 777], [991, 777]],
		[[0, 0, 991 * 991], [0, 991], [0, 991]],
		[[1 * 991, 2 * 991, 3 * 991, 4 * 991], [1, 2, 3, 4], [991]],
		[[4, 11, 20, 30, 20, 11, 4], [1, 2, 3, 4], [4, 3, 2, 1]],
		# 3^100 * 3^28 = 3^128
		[
			natFromString("11790184577738583171520872861412518665678211592275841109096961"),
			natFromString("515377520732011331036461129765621272702107522001"),
			natFromString("22876792454961"),
		],
		# z = 111....1 (70000 digits)
		# x = 10^(99*700) + ... + 10^1400 + 10^700 + 1
		# y = 111....1 (700 digits, larger than Karatsuba threshold on 32-bit and 64-bit)
		[
			natFromString("1".repeat(70000)),
			natFromString("1" + ("0".repeat(699) + "1").repeat(99)),
			natFromString("1".repeat(700)),
		],
		# z = 111....1 (20000 digits)
		# x = 10^10000 + 1
		# y = 111....1 (10000 digits)
		[
			natFromString("1".repeat(20000)),
			natFromString("1" + "0".repeat(9999) + "1"),
			natFromString("1".repeat(10000)),
		],
	]

static func natFromString(s: String) -> PackedInt64Array:
	var x := BigInt.new()
	var err := x.SetString(s)
	assert(err == OK)
	return x._abs

func testFunNN(t: TestingT, msg: String, f: Callable, a: Array) -> void:
	var x := BigInt.new()
	x._abs = a[1]
	var y := BigInt.new()
	y._abs = a[2]
	var z := BigInt.new()
	f.call(z, x, y)
	assert(not z._neg)
	var got := z._abs
	var want: PackedInt64Array = a[0]
	if got != want:
		t.Error("%s%s,\n\tgot z = %s; want %s" % [msg, a, got, want])

func testFunSNN(t: TestingT, msg: String, f: Callable, a: Array) -> void:
	var got: PackedInt64Array = f.call(a[1], a[2])
	var want: PackedInt64Array = a[0]
	if got != want:
		t.Error("%s%s,\n\tgot z = %s; want %s" % [msg, a, got, want])

func TestAdd(t: TestingT) -> void:
	var add := func(z: BigInt, x: BigInt, y: BigInt) -> void:
		z.Add(x, y)
	for a in sumNN:
		testFunNN(t, "add", add, a)
		testFunNN(t, "add", add, [a[0], a[2], a[1]])

func TestSub(t: TestingT) -> void:
	var sub := func(z: BigInt, x: BigInt, y: BigInt) -> void:
		z.Sub(x, y)
	for a in sumNN:
		testFunNN(t, "sub", sub, [a[2], a[0], a[1]])
		testFunNN(t, "sub", sub, [a[1], a[0], a[2]])

func TestNatMul(t0: TestingT) -> void:
	var mulBasic := func(x: PackedInt64Array, y: PackedInt64Array) -> PackedInt64Array:
		return BigInt._test_mul(1000000000, x, y)
	var mulKaratsuba := func(x: PackedInt64Array, y: PackedInt64Array) -> PackedInt64Array:
		return BigInt._test_mul(2, x, y)
	var mul := func(x: PackedInt64Array, y: PackedInt64Array) -> PackedInt64Array:
		return BigInt._test_mul(-1, x, y)
	t0.Run("Basic", func(t: TestingT) -> void:
		for a in prodNN:
			if len(a[2]) >= 100:
				continue
			testFunSNN(t, "mul", mulBasic, a)
			testFunSNN(t, "mul", mulBasic, [a[1], a[0], a[2]]))
	t0.Run("Karatsuba", func(t: TestingT) -> void:
		for a in prodNN:
			testFunSNN(t, "mul", mulKaratsuba, a)
			testFunSNN(t, "mul", mulKaratsuba, [a[1], a[0], a[2]]))
	t0.Run("Mul", func(t: TestingT) -> void:
		for a in prodNN:
			testFunSNN(t, "mul", mul, a)
			testFunSNN(t, "mul", mul, [a[1], a[0], a[2]]))

func testSqr(t: TestingT, basic_sqr_threshold: int, karatsuba_sqr_threshold: int, x: PackedInt64Array) -> void:
	var got := BigInt._test_sqr(basic_sqr_threshold, karatsuba_sqr_threshold, x)
	var want := BigInt._test_mul(-1, x, x)
	if got != want:
		t.Error("basicSqr(%s), got %s, want %s" % [x, got, want])

func TestNatSqr(t0: TestingT) -> void:
	t0.Run("Basic", func(t: TestingT) -> void:
		for a in prodNN:
			if len(a[2]) >= 100:
				continue
			testSqr(t, 0, 1000000000, a[0])
			testSqr(t, 0, 1000000000, a[1])
			testSqr(t, 0, 1000000000, a[2]))
	t0.Run("Karatsuba", func(t: TestingT) -> void:
		for a in prodNN:
			testSqr(t, 2, 2, a[0])
			testSqr(t, 2, 2, a[1])
			testSqr(t, 2, 2, a[2]))
	t0.Run("Sqr", func(t: TestingT) -> void:
		for a in prodNN:
			testSqr(t, -1, -1, a[0])
			testSqr(t, -1, -1, a[1])
			testSqr(t, -1, -1, a[2]))

const mulRangesN: Array[Array] = [
	[0, 0, "0"],
	[1, 1, "1"],
	[1, 2, "2"],
	[1, 3, "6"],
	[10, 10, "10"],
	[0, 100, "0"],
	[0, 1e9, "0"],
	[1, 0, "1"],                    # empty range
	[100, 1, "1"],                  # empty range
	[1, 10, "3628800"],             # 10!
	[1, 20, "2432902008176640000"], # 20!
	[1, 100, "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000"], # 100!
	[-1, -1, "18446744073709551615"],
	[-2, -1, "340282366920938463408034375210639556610"],
	[-3, -1, "6277101735386680761794095221682035635525021984684230311930"],
	[-4, -1, "115792089237316195360799967654821100226821973275796746098729803619699194331160"],
]

func TestMulRangeN(t: TestingT) -> void:
	for i in len(mulRangesN):
		var test := mulRangesN[i]

		var c := BigInt.new()
		c._test_mul_range_unsigned(test[0], test[1])
		var prod := c.String()
		if prod != test[2]:
			t.Error("#%d: got %s; want %s" % [i, prod, test[2]])

static func rndW(r: RandomNumberGenerator) -> int:
	return r.randi() | (r.randi() << 32)

static func rndV(r: RandomNumberGenerator, n: int) -> PackedInt64Array:
	var v: PackedInt64Array
	v.resize(n)
	for i in n:
		v[i] = rndW(r)
	return v

# Construct a vector comprising the same word, usually '0' or 'maximum uint'
static func makeWordVec(e: int, n: int) -> PackedInt64Array:
	var v: PackedInt64Array
	v.resize(n)
	v.fill(e)
	return v

# rndNat returns a random nat value >= 0 of (usually) n words in length.
# In extremely unlikely cases it may be smaller than n words if the top-
# most words are 0.
static func rndNat(r: RandomNumberGenerator, n: int) -> PackedInt64Array:
	return norm(rndV(r, n))

# rndNat1 is like rndNat but the result is guaranteed to be > 0.
static func rndNat1(r: RandomNumberGenerator, n: int) -> PackedInt64Array:
	var x := rndNat(r, n)
	if x.is_empty():
		return [1]
	return x

func benchmarkNatMul(b: TestingB, words: int) -> void:
	var rand := RandomNumberGenerator.new()
	var x := BigInt.new()
	x._abs = rndNat(rand, words)
	var y := BigInt.new()
	y._abs = rndNat(rand, words)
	var z := BigInt.new()
	b.ResetTimer()
	for i in b.N:
		z.Mul(x, y)

const mulBenchSizes: PackedInt64Array = [10, 100, 1000, 10000, 100000]

func BenchmarkNatMul(b0: TestingB) -> void:
	for n in mulBenchSizes:
		b0.Run("%d" % [n], benchmarkNatMul.bind(n))

const leftShiftTests: Array[Array] = [
	[[], 0, []],
	[[], 1, []],
	[[1], 0, [1]],
	[[1], 1, [2]],
	[[INT64_MIN], 1, [0]],
	[[INT64_MIN, 0], 1, [0, 1]],
]

func TestShiftLeft(t: TestingT) -> void:
	t.Error("TODO")
#	for i, test := range leftShiftTests {
#		var z nat
#		z = z.lsh(test.in, test.shift)
#		for j, d := range test.out {
#			if j >= len(z) || z[j] != d {
#				t.Errorf("#%d: got: %v want: %v", i, z, test.out)
#				break
#			}
#		}
#	}

const rightShiftTests: Array[Array] = [
	[[], 0, []],
	[[], 1, []],
	[[1], 0, [1]],
	[[1], 1, []],
	[[2], 1, [1]],
	[[0, 1], 1, [INT64_MIN]],
	[[2, 1, 1], 1, [INT64_MIN + 1, INT64_MIN]],
]

func TestShiftRight(t: TestingT) -> void:
	t.Error("TODO")
#	for i, test := range rightShiftTests {
#		var z nat
#		z = z.rsh(test.in, test.shift)
#		for j, d := range test.out {
#			if j >= len(z) || z[j] != d {
#				t.Errorf("#%d: got: %v want: %v", i, z, test.out)
#				break
#			}
#		}
#	}

#func BenchmarkZeroShifts(b *testing.B) {
#	x := rndNat(800)
#
#	b.Run("Lsh", func(b *testing.B) {
#		for i := 0; i < b.N; i++ {
#			var z nat
#			z.lsh(x, 0)
#		}
#	})
#	b.Run("LshSame", func(b *testing.B) {
#		for i := 0; i < b.N; i++ {
#			x.lsh(x, 0)
#		}
#	})
#
#	b.Run("Rsh", func(b *testing.B) {
#		for i := 0; i < b.N; i++ {
#			var z nat
#			z.rsh(x, 0)
#		}
#	})
#	b.Run("RshSame", func(b *testing.B) {
#		for i := 0; i < b.N; i++ {
#			x.rsh(x, 0)
#		}
#	})
#}

const modWTests32: Array[Array] = [
	["23492635982634928349238759823742", "252341", "220170"],
]

const modWTests64: Array[Array] = [
	["6527895462947293856291561095690465243862946", "524326975699234", "375066989628668"],
]

func runModWTests(t: TestingT, tests: Array[Array]) -> void:
	for i in len(tests):
		var test := tests[i]

		var in_ := BigInt.new()
		var d := BigInt.new()
		var out := BigInt.new()
		in_.SetString(test[0])
		d.SetString(test[1])
		out.SetString(test[2])
		
		var r := BigInt.new()
		r.Mod(in_, d)
		if r.Cmp(out) != 0:
			t.Error("#%d failed: got %s want %s" % [i, r, out])

func TestModW(t: TestingT) -> void:
	runModWTests(t, modWTests32)
	runModWTests(t, modWTests64)

const montgomeryTests: Array[Array] = [
	[
		"0xffffffffffffffffffffffffffffffffffffffffffffffffe",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffe",
		"0xfffffffffffffffffffffffffffffffffffffffffffffffff",
		1,
		"0x1000000000000000000000000000000000000000000",
		"0x10000000000000000000000000000000000",
	],
	[
		"0x000000000ffffff5",
		"0x000000000ffffff0",
		"0x0000000010000001",
		-0xfffffff0000001,
		"0x000000000bfffff4",
		"0x0000000003400001",
	],
	[
		"0x0000000080000000",
		"0x00000000ffffffff",
		"0x1000000000000001",
		0xfffffffffffffff,
		"0x0800000008000001",
		"0x0800000008000001",
	],
	[
		"0x0000000080000000",
		"0x0000000080000000",
		"0xffffffff00000001",
		-0x100000001,
		"0xbfffffff40000001",
		"0xbfffffff40000001",
	],
	[
		"0x0000000080000000",
		"0x0000000080000000",
		"0x00ffffff00000001",
		0xfffffeffffffff,
		"0xbfffff40000001",
		"0xbfffff40000001",
	],
	[
		"0x0000000080000000",
		"0x0000000080000000",
		"0x0000ffff00000001",
		0xfffeffffffff,
		"0xbfff40000001",
		"0xbfff40000001",
	],
	[
		"0x3321ffffffffffffffffffffffffffff00000000000022222623333333332bbbb888c0",
		"0x3321ffffffffffffffffffffffffffff00000000000022222623333333332bbbb888c0",
		"0x33377fffffffffffffffffffffffffffffffffffffffffffff0000000000022222eee1",
		-0x213370edb67ed521,
		"0x04eb0e11d72329dc0915f86784820fc403275bf2f6620a20e0dd344c5cd0875e50deb5",
		"0x0d7144739a7d8e11d72329dc0915f86784820fc403275bf2f61ed96f35dd34dbb3d6a0",
	],
	[
		"0x10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000022222223333333333444444444",
		"0x10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffffffff999999999999999aaabbbbbbbbcccccccccccc",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff33377fffffffffffffffffffffffffffffffffffffffffffff0000000000022222eee1",
		-0x213370edb67ed521,
		"0x5c0d52f451aec609b15da8e5e5626c4eaa88723bdeac9d25ca9b961269400410ca208a16af9c2fb07d7a11c7772cba02c22f9711078d51a3797eb18e691295293284d988e349fa6deba46b25a4ecd9f715",
		"0x92fcad4b5c0d52f451aec609b15da8e5e5626c4eaa88723bdeac9d25ca9b961269400410ca208a16af9c2fb07d799c32fe2f3cc5422f9711078d51a3797eb18e691295293284d8f5e69caf6decddfe1df6",
	],
]

func TestMontgomery(t: TestingT) -> void:
	t.Error("TODO")
#	stk := getStack()
#	defer stk.free()
#
#	one := NewInt(1)
#	_B := new(Int).Lsh(one, _W)
#	for i, test := range montgomeryTests {
#		x := natFromString(test.x)
#		y := natFromString(test.y)
#		m := natFromString(test.m)
#		for len(x) < len(m) {
#			x = append(x, 0)
#		}
#		for len(y) < len(m) {
#			y = append(y, 0)
#		}
#
#		if x.cmp(m) > 0 {
#			_, r := nat(nil).div(stk, nil, x, m)
#			t.Errorf("#%d: x > m (0x%s > 0x%s; use 0x%s)", i, x.utoa(16), m.utoa(16), r.utoa(16))
#		}
#		if y.cmp(m) > 0 {
#			_, r := nat(nil).div(stk, nil, x, m)
#			t.Errorf("#%d: y > m (0x%s > 0x%s; use 0x%s)", i, y.utoa(16), m.utoa(16), r.utoa(16))
#		}
#
#		var out nat
#		if _W == 32 {
#			out = natFromString(test.out32)
#		} else {
#			out = natFromString(test.out64)
#		}
#
#		# t.Logf("#%d: len=%d\n", i, len(m))
#
#		# check output in table
#		xi := &Int{abs: x}
#		yi := &Int{abs: y}
#		mi := &Int{abs: m}
#		p := new(Int).Mod(new(Int).Mul(xi, new(Int).Mul(yi, new(Int).ModInverse(new(Int).Lsh(one, uint(len(m))*_W), mi))), mi)
#		if out.cmp(p.abs.norm()) != 0 {
#			t.Errorf("#%d: out in table=0x%s, computed=0x%s", i, out.utoa(16), p.abs.norm().utoa(16))
#		}
#
#		# check k0 in table
#		k := new(Int).Mod(&Int{abs: m}, _B)
#		k = new(Int).Sub(_B, k)
#		k = new(Int).Mod(k, _B)
#		k0 := Word(new(Int).ModInverse(k, _B).Uint64())
#		if k0 != Word(test.k0) {
#			t.Errorf("#%d: k0 in table=%#x, computed=%#x\n", i, test.k0, k0)
#		}
#
#		# check montgomery with correct k0 produces correct output
#		z := nat(nil).montgomery(x, y, m, k0, len(m))
#		z = z.norm()
#		if z.cmp(out) != 0 {
#			t.Errorf("#%d: got 0x%s want 0x%s", i, z.utoa(16), out.utoa(16))
#		}
#	}

const expNNTests: Array[Array] = [
	["0", "0", "0", "1"],
	["0", "0", "1", "0"],
	["1", "1", "1", "0"],
	["2", "1", "1", "0"],
	["2", "2", "1", "0"],
	["10", "100000000000", "1", "0"],
	["0x8000000000000000", "2", "", "0x40000000000000000000000000000000"],
	["0x8000000000000000", "2", "6719", "4944"],
	["0x8000000000000000", "3", "6719", "5447"],
	["0x8000000000000000", "1000", "6719", "1603"],
	["0x8000000000000000", "1000000", "6719", "3199"],
	[
		"2938462938472983472983659726349017249287491026512746239764525612965293865296239471239874193284792387498274256129746192347",
		"298472983472983471903246121093472394872319615612417471234712061",
		"29834729834729834729347290846729561262544958723956495615629569234729836259263598127342374289365912465901365498236492183464",
		"23537740700184054162508175125554701713153216681790245129157191391322321508055833908509185839069455749219131480588829346291",
	],
	[
		"11521922904531591643048817447554701904414021819823889996244743037378330903763518501116638828335352811871131385129455853417360623007349090150042001944696604737499160174391019030572483602867266711107136838523916077674888297896995042968746762200926853379",
		"426343618817810911523",
		"444747819283133684179",
		"42",
	],
	["375", "249", "388", "175"],
	["375", "18446744073709551801", "388", "175"],
	["0", "0x40000000000000", "0x200", "0"],
	["0xeffffff900002f00", "0x40000000000000", "0x200", "0"],
	["5", "1435700818", "72", "49"],
	["0xffff", "0x300030003000300030003000300030003000302a3000300030003000300030003000300030003000300030003000300030003030623066307f3030783062303430383064303630343036", "0x300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", "0xa3f94c08b0b90e87af637cacc9383f7ea032352b8961fc036a52b659b6c9b33491b335ffd74c927f64ddd62cfca0001"],
]

func TestExpNN(t: TestingT) -> void:
	t.Error("TODO")
#	stk := getStack()
#	defer stk.free()
#
#	for i, test := range expNNTests {
#		x := natFromString(test.x)
#		y := natFromString(test.y)
#		out := natFromString(test.out)
#
#		var m nat
#		if len(test.m) > 0 {
#			m = natFromString(test.m)
#		}
#
#		z := nat(nil).expNN(stk, x, y, m, false)
#		if z.cmp(out) != 0 {
#			t.Errorf("#%d got %s want %s", i, z.utoa(10), out.utoa(10))
#		}
#	}

#func FuzzExpMont(f: TestingF) -> void:
#	f.Fuzz(func(t: TestingT, x1: int, x2: int, x3: int, y1: int, y2: int, y3: int, m1: int, m2: int, m3: int) -> void:
#		if m1 == 0 and m2 == 0 and m3 == 0:
#			return
#		var x := BigInt.new()
#		x._abs = [x1, x2, x3]
#		var y := BigInt.new()
#		y._abs = [y1, y2, y3]
#		var m := BigInt.new()
#		m._abs = [m1, m2, m3]
#		var out := BigInt.new()
#		var want := BigInt.new()
#		out.Exp(x, y, m)
#		want._expSlow(x, y, m)
#		if out.Cmp(want) != 0:
#			t.Error("x = %#x\ny=%#x\nz=%#x\nout=%#x\nwant=%#x\ndc: 16o 16i %X %X %X |p" % [x, y, m, out, want, x, y, m]))

#func BenchmarkExp3Power(b *testing.B) {
#	stk := getStack()
#	defer stk.free()
#
#	const x = 3
#	for _, y := range []Word{
#		0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000,
#	} {
#		b.Run(fmt.Sprintf("%#x", y), func(b *testing.B) {
#			var z nat
#			for i := 0; i < b.N; i++ {
#				z.expWW(stk, x, y)
#			}
#		})
#	}

func fibo(n: int) -> BigInt:
	var f2 := BigInt.new()
	if n == 0:
		return f2
	if n == 1:
		f2.SetUint64(1)
		return f2

	var f0 := fibo(0)
	var f1 := fibo(1)
	for i in range(1, n):
		f2.Add(f0, f1)

		var swap := f0
		f0 = f1
		f1 = f2
		f2 = swap

	return f1

const fiboNums: PackedStringArray = [
	"0",
	"55",
	"6765",
	"832040",
	"102334155",
	"12586269025",
	"1548008755920",
	"190392490709135",
	"23416728348467685",
	"2880067194370816120",
	"354224848179261915075",
]

func TestFibo(t: TestingT) -> void:
	for i in len(fiboNums):
		var want := fiboNums[i]
		var n := i * 10
		var got := str(fibo(n))
		if got != want:
			t.Error("fibo(%d) failed: got %s want %s" % [n, got, want])

func BenchmarkFibo(b: TestingB) -> void:
	for i in b.N:
		fibo(1)
		fibo(10)
		fibo(100)
		fibo(1000)
		fibo(10000)
		fibo(100000)

static var bitTests: Array[Array] = [
	["0", 0, 0],
	["0", 1, 0],
	["0", 1000, 0],

	["0x1", 0, 1],
	["0x10", 0, 0],
	["0x10", 3, 0],
	["0x10", 4, 1],
	["0x10", 5, 0],

	["0x8000000000000000", 62, 0],
	["0x8000000000000000", 63, 1],
	["0x8000000000000000", 64, 0],

	["0x3" + "0".repeat(32), 127, 0],
	["0x3" + "0".repeat(32), 128, 1],
	["0x3" + "0".repeat(32), 129, 1],
	["0x3" + "0".repeat(32), 130, 0],
]

func TestBit(t: TestingT) -> void:
	t.Error("TODO")
#	for i, test := range bitTests {
#		x := natFromString(test.x)
#		if got := x.bit(test.i); got != test.want {
#			t.Errorf("#%d: %s.bit(%d) = %v; want %v", i, test.x, test.i, got, test.want)
#		}
#	}

static var stickyTests: Array[Array] = [
	["0", 0, 0],
	["0", 1, 0],
	["0", 1000, 0],

	["0x1", 0, 0],
	["0x1", 1, 1],

	["0x1350", 0, 0],
	["0x1350", 4, 0],
	["0x1350", 5, 1],

	["0x8000000000000000", 63, 0],
	["0x8000000000000000", 64, 1],

	["0x1" + "0".repeat(100), 400, 0],
	["0x1" + "0".repeat(100), 401, 1],
]

func TestSticky(t: TestingT) -> void:
	t.Error("TODO")
#	for i, test := range stickyTests {
#		x := natFromString(test.x)
#		if got := x.sticky(test.i); got != test.want {
#			t.Errorf("#%d: %s.sticky(%d) = %v; want %v", i, test.x, test.i, got, test.want)
#		}
#		if test.want == 1 {
#			# all subsequent i's should also return 1
#			for d := uint(1); d <= 3; d++ {
#				if got := x.sticky(test.i + d); got != 1 {
#					t.Errorf("#%d: %s.sticky(%d) = %v; want %v", i, test.x, test.i+d, got, 1)
#				}
#			}
#		}
#	}

func benchmarkNatSqr(b: TestingB, words: int) -> void:
	var rand := RandomNumberGenerator.new()
	var x := rndNat(rand, words)
	b.ResetTimer()
	for i in b.N:
		BigInt._test_sqr(-1, -1, x)

const sqrBenchSizes: PackedInt64Array = [
	1, 2, 3, 5, 8, 10, 20, 30, 50, 80,
	100, 200, 300, 500, 800,
	1000, 10000, 100000,
]

func BenchmarkNatSqr(b0: TestingB) -> void:
	for n in sqrBenchSizes:
		b0.Run("%d" % [n], benchmarkNatSqr.bind(n))

const subMod2NTests: Array[Array] = [
	["1", "2", 0, "0"],
	["1", "0", 1, "1"],
	["0", "1", 1, "1"],
	["3", "5", 3, "6"],
	["5", "3", 3, "2"],
	# 2^65, 2^66-1, 2^65 - (2^66-1) + 2^67
	["36893488147419103232", "73786976294838206463", 67, "110680464442257309697"],
	# 2^66-1, 2^65, 2^65-1
	["73786976294838206463", "36893488147419103232", 67, "36893488147419103231"],
]

func TestNatSubMod2N(t: TestingT) -> void:
	t.Error("TODO")
#	for _, mode := range []string{"noalias", "aliasX", "aliasY"} {
#		t.Run(mode, func(t *testing.T) {
#			for _, tt := range subMod2NTests {
#				x0 := natFromString(tt.x)
#				y0 := natFromString(tt.y)
#				want := natFromString(tt.z)
#				x := nat(nil).set(x0)
#				y := nat(nil).set(y0)
#				var z nat
#				switch mode {
#				case "aliasX":
#					z = x
#				case "aliasY":
#					z = y
#				}
#				z = z.subMod2N(x, y, tt.n)
#				if z.cmp(want) != 0 {
#					t.Fatalf("subMod2N(%d, %d, %d) = %d, want %d", x0, y0, tt.n, z, want)
#				}
#				if mode != "aliasX" && x.cmp(x0) != 0 {
#					t.Fatalf("subMod2N(%d, %d, %d) modified x", x0, y0, tt.n)
#				}
#				if mode != "aliasY" && y.cmp(y0) != 0 {
#					t.Fatalf("subMod2N(%d, %d, %d) modified y", x0, y0, tt.n)
#				}
#			}
#		})
#	}

#func BenchmarkNatSetBytes(b *testing.B) {
#	const maxLength = 128
#	lengths := []int{
#		# No remainder:
#		8, 24, maxLength,
#		# With remainder:
#		7, 23, maxLength - 1,
#	}
#	n := make(nat, maxLength/_W) # ensure n doesn't need to grow during the test
#	buf := make([]byte, maxLength)
#	for _, l := range lengths {
#		b.Run(fmt.Sprint(l), func(b *testing.B) {
#			for i := 0; i < b.N; i++ {
#				n.setBytes(buf[:l])
#			}
#		})
#	}

func TestNatDiv(t: TestingT) -> void:
	t.Error("TODO")
#	stk := getStack()
#	defer stk.free()
#
#	sizes := []int{
#		1, 2, 5, 8, 15, 25, 40, 65, 100,
#		200, 500, 800, 1500, 2500, 4000, 6500, 10000,
#	}
#	for _, i := range sizes {
#		for _, j := range sizes {
#			a := rndNat1(i)
#			b := rndNat1(j)
#			# the test requires b >= 2
#			if len(b) == 1 && b[0] == 1 {
#				b[0] = 2
#			}
#			# choose a remainder c < b
#			c := rndNat1(len(b))
#			if len(c) == len(b) && c[len(c)-1] >= b[len(b)-1] {
#				c[len(c)-1] = 0
#				c = c.norm()
#			}
#			# compute x = a*b+c
#			x := nat(nil).mul(stk, a, b)
#			x = x.add(x, c)
#
#			var q, r nat
#			q, r = q.div(stk, r, x, b)
#			if q.cmp(a) != 0 {
#				t.Fatalf("wrong quotient: got %s; want %s for %s/%s", q.utoa(10), a.utoa(10), x.utoa(10), b.utoa(10))
#			}
#			if r.cmp(c) != 0 {
#				t.Fatalf("wrong remainder: got %s; want %s for %s/%s", r.utoa(10), c.utoa(10), x.utoa(10), b.utoa(10))
#			}
#		}
#	}

# TestIssue37499 triggers the edge case of divBasic where
# the inaccurate estimate of the first word's quotient
# happens at the very beginning of the loop.
func TestIssue37499(t: TestingT) -> void:
	t.Error("TODO")
#	stk := getStack()
#	defer stk.free()
#
#	# Choose u and v such that v is slightly larger than u >> N.
#	# This tricks divBasic into choosing 1 as the first word
#	# of the quotient. This works in both 32-bit and 64-bit settings.
#	u := natFromString("0x2b6c385a05be027f5c22005b63c42a1165b79ff510e1706b39f8489c1d28e57bb5ba4ef9fd9387a3e344402c0a453381")
#	v := natFromString("0x2b6c385a05be027f5c22005b63c42a1165b79ff510e1706c")
#
#	q := nat(nil).make(8)
#	q.divBasic(stk, u, v)
#	q = q.norm()
#	if s := string(q.utoa(16)); s != "fffffffffffffffffffffffffffffffffffffffffffffffb" {
#		t.Fatalf("incorrect quotient: %s", s)
#	}

# TestIssue42552 triggers an edge case of recursive division
# where the first division loop is never entered, and correcting
# the remainder takes exactly two iterations in the final loop.
func TestIssue42552(_t: TestingT) -> void:
	var x := BigInt.new()
	x.SetString("0xc23b166884c3869092a520eceedeced2b00847bd256c9cf3b2c5e2227c15bd5e6ee7ef8a2f49236ad0eedf2c8a3b453cf6e0706f64285c526b372c4b1321245519d430540804a50b7ca8b6f1b34a2ec05cdbc24de7599af112d3e3c8db347e8799fe70f16e43c6566ba3aeb169463a3ecc486172deb2d9b80a3699c776e44fef20036bd946f1b4d054dd88a2c1aeb986199b0b2b7e58c42288824b74934d112fe1fc06e06b4d99fe1c5e725946b23210521e209cd507cce90b5f39a523f27e861f9e232aee50c3f585208b4573dcc0b897b6177f2ba20254fd5c50a033e849dee1b3a93bd2dc44ba8ca836cab2c2ae50e50b126284524fa0187af28628ff0face68d87709200329db1392852c8b8963fbe3d05fb1efe19f0ed5ca9fadc2f96f82187c24bb2512b2e85a66333a7e176605695211e1c8e0b9b9e82813e50654964945b1e1e66a90840396c7d10e23e47f364d2d3f660fa54598e18d1ca2ea4fe4f35a40a11f69f201c80b48eaee3e2e9b0eda63decf92bec08a70f731587d4ed0f218d5929285c8b2ccbc497e20db42de73885191fa453350335990184d8df805072f958d5354debda38f5421effaaafd6cb9b721ace74be0892d77679f62a4a126697cd35797f6858193da4ba1770c06aea2e5c59ec04b8ea26749e61b72ecdde403f3bc7e5e546cd799578cc939fa676dfd5e648576d4a06cbadb028adc2c0b461f145b2321f42e5e0f3b4fb898ecd461df07a6f5154067787bf74b5cc5c03704a1ce47494961931f0263b0aac32505102595957531a2de69dd71aac51f8a49902f81f21283dbe8e21e01e5d82517868826f86acf338d935aa6b4d5a25c8d540389b277dd9d64569d68baf0f71bd03dba45b92a7fc052601d1bd011a2fc6790a23f97c6fa5caeea040ab86841f268d39ce4f7caf01069df78bba098e04366492f0c2ac24f1bf16828752765fa523c9a4d42b71109d123e6be8c7b1ab3ccf8ea03404075fe1a9596f1bba1d267f9a7879ceece514818316c9c0583469d2367831fc42b517ea028a28df7c18d783d16ea2436cee2b15d52db68b5dfdee6b4d26f0905f9b030c911a04d078923a4136afea96eed6874462a482917353264cc9bee298f167ac65a6db4e4eda88044b39cc0b33183843eaa946564a00c3a0ab661f2c915e70bf0bb65bfbb6fa2eea20aed16bf2c1a1d00ec55fb4ff2f76b8e462ea70c19efa579c9ee78194b86708fdae66a9ce6e2cf3d366037798cfb50277ba6d2fd4866361022fd788ab7735b40b8b61d55e32243e06719e53992e9ac16c9c4b6e6933635c3c47c8f7e73e17dd54d0dd8aeba5d76de46894e7b3f9d3ec25ad78ee82297ba69905ea0fa094b8667faa2b8885e2187b3da80268aa1164761d7b0d6de206b676777348152b8ae1d4afed753bc63c739a5ca8ce7afb2b241a226bd9e502baba391b5b13f5054f070b65a9cf3a67063bfaa803ba390732cd03888f664023f888741d04d564e0b5674b0a183ace81452001b3fbb4214c77d42ca75376742c471e58f67307726d56a1032bd236610cbcbcd03d0d7a452900136897dc55bb3ce959d10d4e6a10fb635006bd8c41cd9ded2d3dfdd8f2e229590324a7370cb2124210b2330f4c56155caa09a2564932ceded8d92c79664dcdeb87faad7d3da006cc2ea267ee3df41e9677789cc5a8cc3b83add6491561b3047919e0648b1b2e97d7ad6f6c2aa80cab8e9ae10e1f75b1fdd0246151af709d259a6a0ed0b26bd711024965ecad7c41387de45443defce53f66612948694a6032279131c257119ed876a8e805dfb49576ef5c563574115ee87050d92d191bc761ef51d966918e2ef925639400069e3959d8fe19f36136e947ff430bf74e71da0aa5923b00000000")
	var y := BigInt.new()
	y.SetString("0x838332321d443a3d30373d47301d47073847473a383d3030f25b3d3d3e00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002e00000000000000000041603038331c3d32f5303441e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e0e01c0a5459bfc7b9be9fcbb9d2383840464319434707303030f43a32f53034411c0a5459413820878787878787878787878787878787878787878787878787878787878787878787870630303a3a30334036605b923a6101f83638413943413960204337602043323801526040523241846038414143015238604060328452413841413638523c0240384141364036605b923a6101f83638413943413960204334602043323801526040523241846038414143015238604060328452413841413638523c02403841413638433030f25a8b83838383838383838383838383838383837d838383ffffffffffffffff838383838383838383000000000000000000030000007d26e27c7c8b83838383838383838383838383838383837d838383ffffffffffffffff83838383838383838383838383838383838383838383435960f535073030f3343200000000000000011881301938343030fa398383300000002300000000000000000000f11af4600c845252904141364138383c60406032414443095238010241414303364443434132305b595a15434160b042385341ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff47476043410536613603593a6005411c437405fcfcfcfcfcfcfc0000000000005a3b075815054359000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")
	var q := BigInt.new()
	q.Div(x, y)
