// This file is ported from src/math/big/natmul.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_int.h"
#include "godot_big_naturals.h"

using namespace godot;

// Multiplication.

// Operands that are shorter than karatsubaThreshold are multiplied using
// "grade school" multiplication; for longer operands the Karatsuba algorithm
// is used.
static constexpr int64_t karatsubaThreshold = 40; // see calibrate_test.go

// Operands that are shorter than basicSqrThreshold are squared using
// "grade school" multiplication; for operands longer than karatsubaSqrThreshold
// we use the Karatsuba algorithm optimized for x == y.
static constexpr int64_t basicSqrThreshold = 12; // see calibrate_test.go
static constexpr int64_t karatsubaSqrThreshold = 80; // see calibrate_test.go

// mul sets z = x*y, using stk for temporary storage.
// The caller may pass stk == nil to request that mul obtain and release one itself.
void BigNat::mul(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	const int64_t m = p_x.array.size();
	const int64_t n = p_y.array.size();

	if (m < n) {
		mul(p_y, p_x);
		return;
	}
	if (m == 0 || n == 0) {
		array.clear();
		return;
	}
	if (n == 1) {
		mulAddWW(p_x, p_y[0], 0);
		return;
	}
	// m >= n > 1

	// use basic multiplication if the numbers are small
	if (n < karatsubaThreshold) {
		array.resize(m + n);
		basicMul(*this, p_x, p_y);
		norm();
		return;
	}

	array.resize(n + n);

	// Let x = x1:x0 where x0 is the same length as y.
	// Compute z = x0*y and then add in x1*y in sections
	// if needed.
	karatsuba(*this, BigNat{ p_x.array.slice(0, n) }, p_y);

	if (n < m) {
		array.resize(m + n);
		BigNat t;
		for (int64_t i = n; i < m; i += n) {
			t.mul(BigNat{ p_x.array.slice(i, Math::min(i + n, p_x.array.size())) }, p_y);
			addTo(*this, i, t);
		}
	}

	norm();
}

// sqr sets z = x*x, using stk for temporary storage.
// The caller may pass stk == nil to request that sqr obtain and release one itself.
void BigNat::sqr(BigNat p_x) {
	const int64_t n = p_x.array.size();
	array.resize(2 * n);
	if (n == 0) {
		return;
	}

	if (n == 1) {
		const BigWord d = p_x[0];
		mulWW(d, d, (*this)[1], (*this)[0]);
		norm();
		return;
	}

	if (n < basicSqrThreshold && n < karatsubaSqrThreshold) {
		basicMul(*this, p_x, p_x);
		norm();
		return;
	}

	if (n < karatsubaSqrThreshold) {
		basicSqr(*this, p_x);
		norm();
		return;
	}

	karatsubaSqr(*this, p_x);
	norm();
}

// basicSqr sets z = x*x and is asymptotically faster than basicMul
// by about a factor of 2, but slower for small arguments due to overhead.
// Requirements: len(x) > 0, len(z) == 2*len(x)
// The (non-normalized) result is placed in z.
void BigNat::basicSqr(BigNat &z, BigNat x) {
	const int64_t n = x.array.size();
	if (n < basicSqrThreshold) {
		basicMul(z, x, x);
		return;
	}

	BigNat t;
	t.array.resize(2 * n);

	mulWW(x[0], x[0], z[1], z[0]); // the initial square
	for (int64_t i = 1; i < n; i++) {
		const BigWord d = x[i];
		// z collects the squares x[i] * x[i]
		mulWW(d, d, z[(2 * i) + 1], z[2 * i]);
		// t collects the products x[i] * x[j] where j < i
		BigNat tempt{ t.array.slice(i, 2 * i) };
		t[2 * i] = addMulVVWW(tempt, tempt, BigNat{ x.array.slice(0, i) }, d, 0);
		for (int64_t j = 0; j < i; j++) {
			t[i + j] = tempt[j];
		}
	}
	BigNat tempt{ t.array.slice(1, (2 * n) - 1) };
	t[(2 * n) - 1] = lshVU(tempt, tempt, 1); // double the j < i products
	for (int64_t j = 0; j < (2 * n) - 2; j++) {
		t[1 + j] = tempt[j];
	}
	addVV(z, z, t); // combine the result
}

// mulAddWW returns z = x*y + r.
void BigNat::mulAddWW(BigNat p_x, BigWord p_y, BigWord p_r) { // NOLINT(performance-unnecessary-value-param)
	int64_t m = p_x.array.size();
	if (m == 0 || p_y == 0) {
		setUint64(p_r); // result is r
		return;
	}
	// m > 0

	array.resize(m);
	array.append(mulAddVWW(*this, p_x, p_y, p_r));

	norm();
}

// basicMul multiplies x and y and leaves the result in z.
// The (non-normalized) result is placed in z[0 : len(x) + len(y)].
void BigNat::basicMul(BigNat &z, BigNat x, BigNat y) { // NOLINT(performance-unnecessary-value-param)
	for (int64_t i = 0; i < x.array.size() + y.array.size(); i++) {
		z[i] = 0; // initialize z
	}
	for (int64_t i = 0; i < y.array.size(); i++) {
		const BigWord d = y[i];
		if (d != 0) {
			BigNat tempz{ z.array.slice(i, i + x.array.size()) };
			z[x.array.size() + i] = addMulVVWW(tempz, tempz, x, d, 0);
			for (int64_t j = 0; j < x.array.size(); j++) {
				z[i + j] = tempz[j];
			}
		}
	}
}

#ifdef DEBUG_ENABLED
// ifmt returns the debug formatting of the Int x: 0xHEX.
static String ifmt(const BigNat &x) {
	String s = x.utoa(16);
	String t;
	if (s.is_empty()) { // happens for denormalized zero
		s = "0x0";
	}
	String neg;
	if (s[0] == '-') {
		neg = "-";
		s = s.substr(1);
	}

	// Add _ between words.
	constexpr int64_t D = 64 / 4; // digits per chunk
	while (s.length() > D) {
		t = s.substr(s.length() - D) + "_" + t;
		s = s.substr(0, s.length() - D);
	}
	return neg + s + t;
}
static String ifmt(const Ref<BigInt> &x) {
	String neg = x->_neg ? "-" : "";
	return neg + ifmt(x->_abs);
}

// trace prints a single debug value.
static void trace(const String &name, const BigNat &x) {
	print_line(name, "=", ifmt(x));
}
static void trace(const String &name, const Ref<BigInt> &x) {
	print_line(name, "=", ifmt(x));
}
#endif

// karatsuba multiplies x and y,
// writing the (non-normalized) result to z.
// x and y must have the same length n,
// and z must have length twice that.
void BigNat::karatsuba(BigNat &z, BigNat x, BigNat y) { // NOLINT(performance-unnecessary-value-param)
	const int64_t n = y.array.size();
	CRASH_COND(x.array.size() != n || z.array.size() != 2 * n);

	// Fall back to basic algorithm if small enough.
	if (n < karatsubaThreshold || n < 2) {
		basicMul(z, x, y);
		return;
	}

	// Let the notation x1:x0 denote the nat (x1<<N)+x0 for some N,
	// and similarly z2:z1:z0 = (z2<<2N)+(z1<<N)+z0.
	//
	// (Note that z0, z1, z2 might be ≥ 2**N, in which case the high
	// bits of, say, z0 are being added to the low bits of z1 in this notation.)
	//
	// Karatsuba multiplication is based on the observation that
	//
	//	x1:x0 * y1:y0 = x1*y1:(x0*y1+y0*x1):x0*y0
	//	              = x1*y1:((x0-x1)*(y1-y0)+x1*y1+x0*y0):x0*y0
	//
	// The second form uses only three half-width multiplications
	// instead of the four that the straightforward first form does.
	//
	// We call the three pieces z0, z1, z2:
	//
	//	z0 = x0*y0
	//	z2 = x1*y1
	//	z1 = (x0-x1)*(y1-y0) + z0 + z2

	const int64_t n2 = (n + 1) / 2;
	Ref<BigInt> x0{ memnew(BigInt) };
	Ref<BigInt> x1{ memnew(BigInt) };
	Ref<BigInt> y0{ memnew(BigInt) };
	Ref<BigInt> y1{ memnew(BigInt) };
	Ref<BigInt> z0{ memnew(BigInt) };
	Ref<BigInt> z1{ memnew(BigInt) };
	Ref<BigInt> z2{ memnew(BigInt) };
	Ref<BigInt> tx{ memnew(BigInt) };
	Ref<BigInt> ty{ memnew(BigInt) };

	x0->_abs.array = x.array.slice(0, n2);
	x0->_abs.norm();
	x1->_abs.array = x.array.slice(n2);
	x1->_abs.norm();

	y0->_abs.array = y.array.slice(0, n2);
	y0->_abs.norm();
	y1->_abs.array = y.array.slice(n2);
	y1->_abs.norm();

	tx->Sub(x0, x1);
	ty->Sub(y1, y0);
	z1->Mul(tx, ty);

	z0->Mul(x0, y0);
	z2->Mul(x1, y1);
	z1->Add(z1, z0);
	z1->Add(z1, z2);

	z.array = z0->_abs.array;
	DEV_ASSERT(z.array.size() <= 2 * n2);
	z.array.resize(2 * n2);
	z.array.append_array(z2->_abs.array);
	DEV_ASSERT(z.array.size() <= 2 * n);
	z.array.resize(2 * n);
	addTo(z, n2, z1->_abs);

	// Debug mode: double-check answer and print trace on failure.
#ifdef DEBUG_ENABLED
	BigNat zz;
	zz.array.resize(z.array.size());
	basicMul(zz, x, y);
	if (z.cmp(zz) != 0) {
		print_line("karatsuba wrong");
		trace("x ", x);
		trace("y ", y);
		trace("z ", z);
		trace("zz", zz);
		trace("x0", x0);
		trace("x1", x1);
		trace("y0", y0);
		trace("y1", y1);
		trace("tx", tx);
		trace("ty", ty);
		trace("z0", z0);
		trace("z1", z1);
		trace("z2", z2);
		CRASH_NOW_MSG("karatsuba");
	}
#endif
}

// karatsubaSqr squares x,
// writing the (non-normalized) result to z.
// z must have length 2*len(x).
// It is analogous to [karatsuba] but can run faster
// knowing both multiplicands are the same value.
void BigNat::karatsubaSqr(BigNat &z, BigNat x) { // NOLINT(performance-unnecessary-value-param)
	const int64_t n = x.array.size();
	CRASH_COND(z.array.size() != 2 * n);

	if (n < karatsubaSqrThreshold || n < 2) {
		basicSqr(z, x);
		return;
	}

	// Recall that for karatsuba we want to compute:
	//
	//	x1:x0 * y1:y0 = x1y1:(x0y1+y0x1):x0y0
	//                = x1y1:((x0-x1)*(y1-y0)+x1y1+x0y0):x0y0
	//	              = z2:z1:z0
	// where:
	//
	//	z0 = x0y0
	//	z2 = x1y1
	//	z1 = (x0-x1)*(y1-y0) + z0 + z2
	//
	// When x = y, these simplify to:
	//
	//	z0 = x0²
	//	z2 = x1²
	//	z1 = z0 + z2 - (x0-x1)²

	const int64_t n2 = (n + 1) / 2;
	Ref<BigInt> x0{ memnew(BigInt) };
	Ref<BigInt> x1{ memnew(BigInt) };
	Ref<BigInt> z0{ memnew(BigInt) };
	Ref<BigInt> z1{ memnew(BigInt) };
	Ref<BigInt> z2{ memnew(BigInt) };
	Ref<BigInt> tx{ memnew(BigInt) };

	x0->_abs.array = x.array.slice(0, n2);
	x0->_abs.norm();
	x1->_abs.array = x.array.slice(n2);
	x1->_abs.norm();

	tx->Sub(x0, x1);
	z1->_abs.sqr(tx->_abs);
	z1->_neg = true;

	z0->_abs.sqr(x0->_abs);
	z2->_abs.sqr(x1->_abs);
	z1->Add(z1, z0);
	z1->Add(z1, z2);

	z.array = z0->_abs.array;
	DEV_ASSERT(z.array.size() <= 2 * n2);
	z.array.resize(2 * n2);
	z.array.append_array(z2->_abs.array);
	DEV_ASSERT(z.array.size() <= 2 * n);
	z.array.resize(2 * n);
	addTo(z, n2, z1->_abs);

	// Debug mode: double-check answer and print trace on failure.
#ifdef DEBUG_ENABLED
	BigNat zz;
	basicSqr(zz, x);
	if (z.cmp(zz) != 0) {
		print_line("karatsubaSqr wrong");
		trace("x ", x);
		trace("z ", z);
		trace("zz", zz);
		trace("x0", x0);
		trace("x1", x1);
		trace("z0", z0);
		trace("z1", z1);
		trace("z2", z2);
		CRASH_NOW_MSG("karatsubaSqr");
	}
#endif
}
