// This file is ported from src/math/big/natmul.go in Go 1.27.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Multiplication.

#include "godot_big_debug.h"
#include "godot_big_int.h"

using namespace godot;

// Operands that are shorter than karatsubaThreshold are multiplied using
// "grade school" multiplication; for longer operands the Karatsuba algorithm
// is used.
CALIBRATION_SPECIFIERS uint64_t karatsubaThreshold = 40; // see calibrate_test.go

// Operands that are shorter than basicSqrThreshold are squared using
// "grade school" multiplication; for operands longer than karatsubaSqrThreshold
// we use the Karatsuba algorithm optimized for x == y.
CALIBRATION_SPECIFIERS uint64_t basicSqrThreshold = 12; // see calibrate_test.go
CALIBRATION_SPECIFIERS uint64_t karatsubaSqrThreshold = 80; // see calibrate_test.go

PackedInt64Array BigNat::test_mul_bind(uint64_t p_karatsuba_threshold, const PackedInt64Array &p_x, const PackedInt64Array &p_y) {
#ifdef CALIBRATION_SPECIFIERS_MUTABLE
	const uint64_t old_karatsuba_threshold = karatsubaThreshold;
	karatsubaThreshold = p_karatsuba_threshold;
#else
	WARN_PRINT("BigInt._test_mul does not change thresholds on release builds");
#endif

	BigNat product;
	product.mul(BigNat{ p_x }, BigNat{ p_y });

#ifdef CALIBRATION_SPECIFIERS_MUTABLE
	karatsubaThreshold = old_karatsuba_threshold;
#endif

	return product.array;
}

PackedInt64Array BigNat::test_sqr_bind(uint64_t p_basic_sqr_threshold, uint64_t p_karatsuba_sqr_threshold, const PackedInt64Array &p_x) {
#ifdef CALIBRATION_SPECIFIERS_MUTABLE
	const uint64_t old_basic_sqr_threshold = basicSqrThreshold;
	basicSqrThreshold = p_basic_sqr_threshold;
	const uint64_t old_karatsuba_sqr_threshold = karatsubaSqrThreshold;
	karatsubaSqrThreshold = p_karatsuba_sqr_threshold;
#else
	WARN_PRINT("BigInt._test_sqr does not change thresholds on release builds");
#endif

	BigNat square;
	square.sqr(BigNat{ p_x });

#ifdef CALIBRATION_SPECIFIERS_MUTABLE
	basicSqrThreshold = old_basic_sqr_threshold;
	karatsubaSqrThreshold = old_karatsuba_sqr_threshold;
#endif

	return square.array;
}

// mul sets z = x*y, using stk for temporary storage.
// The caller may pass stk == nil to request that mul obtain and release one itself.
void BigNat::mul(BigNat p_x, BigNat p_y) {
	uint64_t m = p_x.array.size();
	uint64_t n = p_y.array.size();

	if (m < n) {
		SWAP(m, n);
		SWAP(p_x, p_y);
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

	array.resize(m + n);

	// use basic multiplication if the numbers are small
	if (n < karatsubaThreshold) {
		basicMul(slice(), p_x.cslice(), p_y.cslice());
		norm();
		return;
	}

	// Let x = x1:x0 where x0 is the same length as y.
	// Compute z = x0*y and then add in x1*y in sections
	// if needed.
	karatsuba(slice(0, 2 * n), p_x.cslice(0, n), p_y.cslice());

	if (n < m) {
		slice(2 * n).fill(0);
		BigNat t;
		t.array.resize(2 * n);
		for (uint64_t i = n; i < m; i += n) {
			BigNat xi;
			xi.array = p_x.array.slice(i, Math::min(i + n, static_cast<uint64_t>(p_x.array.size())));
			t.mul(xi, p_y);
			addTo(slice(i), t.cslice());
		}
	}

	norm();
}

// sqr sets z = x*x, using stk for temporary storage.
// The caller may pass stk == nil to request that sqr obtain and release one itself.
void BigNat::sqr(BigNat p_x) {
	const uint64_t n = p_x.array.size();

	if (n == 0) {
		array.clear();
		return;
	}

	if (n == 1) {
		const BigWord d = p_x[0];
		array.resize(2);
		mulWW(d, d, operator[](1), operator[](0));
		norm();
		return;
	}

	array.resize(2 * n);

	if (n < basicSqrThreshold && n < karatsubaSqrThreshold) {
		basicMul(slice(), p_x.cslice(), p_x.cslice());
		norm();
		return;
	}

	if (n < karatsubaSqrThreshold) {
		basicSqr(slice(), p_x.cslice());
		norm();
		return;
	}

	karatsubaSqr(slice(), p_x.cslice());
	norm();
}

// basicSqr sets z = x*x and is asymptotically faster than basicMul
// by about a factor of 2, but slower for small arguments due to overhead.
// Requirements: len(x) > 0, len(z) == 2*len(x)
// The (non-normalized) result is placed in z.
void BigNat::basicSqr(Slice r_z, const CSlice &p_x) {
	const int64_t n = p_x.size();
	if (n < basicSqrThreshold) {
		basicMul(r_z, p_x, p_x);
		return;
	}

	BigWord *z = r_z.array;
	const BigWord *x = p_x.ptr();

	BigNat t;
	t.array.resize(2 * n);

	// intentionally defeat copy-on-write
	Slice ts = t.slice();

	mulWW(x[0], x[0], z[1], z[0]); // the initial square
	for (int64_t i = 1; i < n; i++) {
		const BigWord d = x[i];

		// z collects the squares x[i] * x[i]
		mulWW(d, d, z[(2 * i) + 1], z[2 * i]);

		// t collects the products x[i] * x[j] where j < i
		ts[2 * i] = addMulVVWW(ts.slice(i, 2 * i), t.cslice(i, 2 * i), { x, static_cast<uint64_t>(i) }, d, 0);
	}

	// double the j < i products
	ts[(2 * n) - 1] = lshVU(ts.slice(1, (2 * n) - 1), t.cslice(1, (2 * n) - 1), 1);

	// combine the result
	addVV(r_z, r_z.cslice(), t.cslice());
}

// mulAddWW returns z = x*y + r.
void BigNat::mulAddWW(BigNat p_x, BigWord p_y, BigWord p_r) { // NOLINT(performance-unnecessary-value-param)
	const int64_t m = p_x.array.size();
	if (m == 0 || p_y == 0) {
		setUint64(p_r); // result is r

		return;
	}
	// m > 0

	array.resize(m + 1);
	operator[](m) = mulAddVWW(slice(0, m), p_x.cslice(), p_y, p_r);

	norm();
}

// basicMul multiplies x and y and leaves the result in z.
// The (non-normalized) result is placed in z[0 : len(x) + len(y)].
void BigNat::basicMul(Slice r_z, const CSlice &p_x, const CSlice &p_y) {
	const uint64_t n = p_x.size();
	const uint64_t m = p_y.size();

	r_z.slice(0, p_x.size() + p_y.size()).fill(0); // initialize z

	BigWord *z = r_z.array;
	const BigWord *y = p_y.ptr();

	for (uint64_t i = 0; i < m; i++) {
		const BigWord d = y[i];
		if (d != 0) {
			z[p_x.size() + i] = addMulVVWW(r_z.slice(i, i + n), { z + i, n }, p_x, d, 0);
		}
	}
}

// ifmt returns the debug formatting of the Int x: 0xHEX.
static String ifmt(const BigNat &p_x) {
	String s = p_x.utoa(16).get_string_from_ascii();
	if (s.is_empty()) { // happens for denormalized zero
		s = "0x0";
	}

	String t;
	// Add _ between words.
	while (s.length() > 16) {
		t = s.substr(s.length() - 16) + "_" + t;
		s = s.substr(0, s.length() - 16);
	}

	return s + t;
}
static String ifmt(const Ref<BigInt> &p_x) {
	String s = ifmt(p_x->_abs);

	if (p_x->_neg) {
		s = "-" + s;
	}

	return s;
}

// trace prints a single debug value.
static void trace(const String &p_name, const BigNat &p_x) {
	print_line(p_name, "=", ifmt(p_x));
}
static void trace(const String &p_name, const Ref<BigInt> &p_x) {
	print_line(p_name, "=", ifmt(p_x));
}

// karatsuba multiplies x and y,
// writing the (non-normalized) result to z.
// x and y must have the same length n,
// and z must have length twice that.
void BigNat::karatsuba(Slice r_z, const CSlice &p_x, const CSlice &p_y) {
	const uint64_t n = p_y.size();
	CRASH_COND_MSG(p_x.size() != n || r_z.size() != 2 * n, "bad karatsuba length");

	// Fall back to basic algorithm if small enough.
	if (n < karatsubaThreshold || n < 2) {
		basicMul(r_z, p_x, p_y);
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

	const uint64_t n2 = (n + 1) / 2;
	Ref<BigInt> x0{ memnew(BigInt) };
	Ref<BigInt> x1{ memnew(BigInt) };
	Ref<BigInt> y0{ memnew(BigInt) };
	Ref<BigInt> y1{ memnew(BigInt) };
	Ref<BigInt> z0{ memnew(BigInt) };
	Ref<BigInt> z1{ memnew(BigInt) };
	Ref<BigInt> z2{ memnew(BigInt) };
	Ref<BigInt> tx{ memnew(BigInt) };
	Ref<BigInt> ty{ memnew(BigInt) };

	x0->_abs.array.resize(n2);
	memcpy(x0->_abs.ptrw(), p_x.ptr(), sizeof(BigWord) * n2);
	x0->_abs.norm();

	x1->_abs.array.resize(n - n2);
	memcpy(x1->_abs.ptrw(), p_x.ptr() + n2, sizeof(BigWord) * (n - n2));
	x1->_abs.norm();

	y0->_abs.array.resize(n2);
	memcpy(y0->_abs.ptrw(), p_y.ptr(), sizeof(BigWord) * n2);
	y0->_abs.norm();

	y1->_abs.array.resize(n - n2);
	memcpy(y1->_abs.ptrw(), p_y.ptr() + n2, sizeof(BigWord) * (n - n2));
	y1->_abs.norm();

	z0->_abs.array.resize(2 * n2);
	z1->_abs.array.resize((2 * n2) + 1);
	z2->_abs.array.resize((2 * n) - (2 * n2));
	tx->_abs.array.resize(n2);
	ty->_abs.array.resize(n2);

	tx->Sub(x0, x1);
	ty->Sub(y1, y0);

	z1->Mul(tx, ty);

	r_z.fill(0);
	z0->Mul(x0, y0);
	memcpy(r_z.array, z0->_abs.ptr(), z0->_abs.array.size() * sizeof(BigWord));
	z2->Mul(x1, y1);
	memcpy(r_z.array + (2 * n2), z2->_abs.ptr(), z2->_abs.array.size() * sizeof(BigWord));

	z1->Add(z1, z0);
	z1->Add(z1, z2);

	addTo(r_z.slice(n2), z1->_abs.cslice());

	// Debug mode: double-check answer and print trace on failure.
#ifdef DEBUG_KARATSUBA_ENABLED
	{
		BigNat z{ r_z };
		BigNat zz;
		zz.array.resize(r_z.length);
		basicMul(zz.slice(), p_x, p_y);
		if (z.cmp(zz) != 0) {
			trace("x ", BigNat{ p_x });
			trace("y ", BigNat{ p_y });
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
			CRASH_NOW_MSG("karatsuba wrong");
		}
	}
#endif
}

// karatsubaSqr squares x,
// writing the (non-normalized) result to z.
// z must have length 2*len(x).
// It is analogous to [karatsuba] but can run faster
// knowing both multiplicands are the same value.
void BigNat::karatsubaSqr(Slice r_z, const CSlice &p_x) {
	const uint64_t n = p_x.size();
	CRASH_COND_MSG(r_z.size() != 2 * n, "bad karatsubaSqr length");

	if (n < karatsubaSqrThreshold || n < 2) {
		basicSqr(r_z, p_x);
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

	const uint64_t n2 = (n + 1) / 2;
	Ref<BigInt> x0{ memnew(BigInt) };
	Ref<BigInt> x1{ memnew(BigInt) };
	Ref<BigInt> z0{ memnew(BigInt) };
	Ref<BigInt> z1{ memnew(BigInt) };
	Ref<BigInt> z2{ memnew(BigInt) };
	Ref<BigInt> tx{ memnew(BigInt) };

	x0->_abs.array.resize(n2);
	memcpy(x0->_abs.ptrw(), p_x.ptr(), sizeof(BigWord) * n2);
	x0->_abs.norm();

	x1->_abs.array.resize(n - n2);
	memcpy(x1->_abs.ptrw(), p_x.ptr() + n2, sizeof(BigWord) * (n - n2));
	x1->_abs.norm();

	z0->_abs.array.resize(2 * n2);
	z1->_abs.array.resize((2 * n2) + 1);
	z2->_abs.array.resize((2 * n) - (2 * n2));
	tx->_abs.array.resize(n2);

	tx->Sub(x0, x1);
	z1->_abs.sqr(tx->_abs);
	z1->_neg = true;

	r_z.fill(0);
	z0->_abs.sqr(x0->_abs);
	memcpy(r_z.array, z0->_abs.ptr(), z0->_abs.array.size() * sizeof(BigWord));
	z2->_abs.sqr(x1->_abs);
	memcpy(r_z.array + (2 * n2), z2->_abs.ptr(), z2->_abs.array.size() * sizeof(BigWord));

	z1->Add(z1, z0);
	z1->Add(z1, z2);

	addTo(r_z.slice(n2), z1->_abs.cslice());

	// Debug mode: double-check answer and print trace on failure.
#ifdef DEBUG_KARATSUBA_ENABLED
	{
		BigNat z{ r_z };
		BigNat zz;
		zz.array.resize(r_z.size());
		basicSqr(zz.slice(), p_x);
		if (z.cmp(zz) != 0) {
			trace("x ", BigNat{ p_x });
			trace("z ", z);
			trace("zz", zz);
			trace("x0", x0);
			trace("x1", x1);
			trace("z0", z0);
			trace("z1", z1);
			trace("z2", z2);
			CRASH_NOW_MSG("karatsubaSqr wrong");
		}
	}
#endif
}
