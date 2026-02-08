// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"
#include "godot_big_int.h"

// Multiplication.

// Operands that are shorter than karatsubaThreshold are multiplied using
// "grade school" multiplication; for longer operands the Karatsuba algorithm
// is used.
static constexpr int64_t karatsubaThreshold = 40; // see calibrate_test.go

// Operands that are shorter than basicSqrThreshold are squared using
// "grade school" multiplication; for operands longer than karatsubaSqrThreshold
// we use the Karatsuba algorithm optimized for x == y.
static constexpr int64_t basicSqrThreshold = 12;     // see calibrate_test.go
static constexpr int64_t karatsubaSqrThreshold = 80; // see calibrate_test.go

// mul sets z = x*y, using stk for temporary storage.
// The caller may pass stk == nil to request that mul obtain and release one itself.
void nat_mul(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();

	if (m < n) {
		std::swap(x, y);
		std::swap(m, n);
	}

	if (m == 0 || n == 0) {
		z.clear();
		return;
	}

	if (n == 1) {
		nat_mulAddWW(z, x, y[0], 0);
		return;
	}

	// m >= n > 1

	z.resize(m + n);

	// use basic multiplication if the numbers are small
	if (n < karatsubaThreshold) {
		nat_basicMul(z, x, y);
		nat_norm(z);
		return;
	}

	// Let x = x1:x0 where x0 is the same length as y.
	// Compute z = x0*y and then add in x1*y in sections
	// if needed.
	z.resize(2 * n);
	nat_karatsuba(z, x.slice(0, n), y);

	if (n < m) {
		z.resize(m + n);
		PackedInt64Array t;
		for (int64_t i = n; i < m; i += n) {
			nat_mul(t, x.slice(i, Math::min(i + n, m)), y);
			nat_addTo(z, i, t);
		}
	}

	nat_norm(z);
}

// sqr sets z = x*x, using stk for temporary storage.
// The caller may pass stk == nil to request that sqr obtain and release one itself.
void nat_sqr(PackedInt64Array &z, PackedInt64Array x) {
	const int64_t n = x.size();
	if (n == 0) {
		z.clear();
		return;
	}

	if (n == 1) {
		const uint64_t d = x[0];
		uint64_t z1, z0;
		z.resize(2);
		nat_mulWW(d, d, z1, z0);
		z[1] = z1;
		z[0] = z0;
		nat_norm(z);
		return;
	}

	z.resize(2 * n);

	if (n < basicSqrThreshold && n < karatsubaSqrThreshold) {
		nat_basicMul(z, x, x);
		nat_norm(z);
		return;
	}

	if (n < karatsubaSqrThreshold) {
		nat_basicSqr(z, x);
		nat_norm(z);
		return;
	}

	nat_karatsubaSqr(z, x);
	nat_norm(z);
}

// basicSqr sets z = x*x and is asymptotically faster than basicMul
// by about a factor of 2, but slower for small arguments due to overhead.
// Requirements: len(x) > 0, len(z) == 2*len(x)
// The (non-normalized) result is placed in z.
void nat_basicSqr(PackedInt64Array &z, PackedInt64Array x) {
	const int64_t n = x.size();
	CRASH_COND(z.size() != 2 * n);

	if (n < basicSqrThreshold) {
		nat_basicMul(z, x, x);
		return;
	}

	PackedInt64Array t;
	t.resize(2 * n);

	uint64_t z1, z0;
	nat_mulWW(x[0], x[0], z1, z0); // the initial square
	z[1] = z1;
	z[0] = z0;
	for (int64_t i = 1; i < n; i++) {
		const uint64_t d = uint64_t(x[i]);

		// z collects the squares x[i] * x[i]
		nat_mulWW(d, d, z1, z0);
		z[2 * i + 1] = z1;
		z[2 * i] = z0;

		// t collects the products x[i] * x[j] where j < i
		t[2 * i] = nat_addMulVVWW(t, i, t, i, x, 0, d, 0, i);
	}
	t[2 * n - 1] = nat_lshVU(t, 1, t, 1, 1, 2 * n - 2); // double the j < i products
	nat_addVV(z, 0, z, 0, t, 0, 2 * n);                 // combine the result
}

// mulAddWW returns z = x*y + r.
void nat_mulAddWW(PackedInt64Array &z, PackedInt64Array x, uint64_t y, uint64_t r) {
	const int64_t m = x.size();
	if (m == 0 || y == 0) {
		nat_setWord(z, r); // result is r
		return;
	}
	// m > 0

	z.resize(m + 1);
	z[m] = nat_mulAddVWW(z, 0, x, 0, y, r, m);
	nat_norm(z);
}

// basicMul multiplies x and y and leaves the result in z.
// The (non-normalized) result is placed in z[0 : len(x) + len(y)].
void nat_basicMul(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	nat_clear(z, 0, x.size() + y.size()); // initialize z
	for (int64_t i = 0; i < y.size(); i++) {
		const uint64_t d = y[i];
		if (d != 0) {
			z[x.size() + i] = nat_addMulVVWW(z, i, z, i, x, 0, d, 0, x.size());
		}
	}
}

#ifdef GODOT_BIG_DEBUG_KARATSUBA
// ifmt returns the debug formatting of the Int x: 0xHEX.
static String ifmt(Ref<BigInt> x) {
	String neg, s = x->Text(16), t;
	if (s == "") { // happens for denormalized zero
		s = "0x0";
	}
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
#endif

// karatsuba multiplies x and y,
// writing the (non-normalized) result to z.
// x and y must have the same length n,
// and z must have length twice that.
void nat_karatsuba(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	const int64_t n = x.size();

	CRASH_COND(y.size() != n);
	CRASH_COND(z.size() != n * 2);

	// Fall back to basic algorithm if small enough.
	if (n < karatsubaThreshold || n < 2) {
		nat_basicMul(z, x, y);
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

	PackedInt64Array x0n = x.slice(0, n2);
	nat_norm(x0n);
	PackedInt64Array x1n = x.slice(n2);
	nat_norm(x1n);
	PackedInt64Array y0n = y.slice(0, n2);
	nat_norm(y0n);
	PackedInt64Array y1n = y.slice(n2);
	nat_norm(y1n);

	Ref<BigInt> x0, x1, y0, y1, z0, z1, z2, tx, ty;
	x0.instantiate();
	x0->_set_abs(x0n);
	x1.instantiate();
	x1->_set_abs(x1n);
	y0.instantiate();
	y0->_set_abs(y0n);
	y1.instantiate();
	y1->_set_abs(y1n);

	z0.instantiate();
	z2.instantiate();

	// Allocate temporary storage for z1; repurpose z0 to hold tx and ty.
	z1.instantiate();
	tx.instantiate();
	ty.instantiate();

	tx->Sub(x0, x1);
	ty->Sub(y1, y0);
	z1->Mul(tx, ty);

	z0->Mul(x0, y0);
	z2->Mul(x1, y1);
	z1->Add(z1, z0);
	z1->Add(z1, z2);
	z.fill(0);
	nat_copy(z, 0, z0->_get_abs(), 0, z0->_get_abs().size());
	nat_copy(z, n2 * 2, z2->_get_abs(), 0, z2->_get_abs().size());
	nat_addTo(z, n2, z1->_get_abs());

#ifdef GODOT_BIG_DEBUG_KARATSUBA
	// Debug mode: double-check answer and print trace on failure.
	PackedInt64Array zz;
	zz.resize(z.size());
	nat_basicMul(zz, x, y);
	if (nat_cmp(z, zz) != 0) {
		// All the temps were aliased to z and gone. Recompute.
		z0->Mul(x0, y0);
		tx->Sub(x1, x0);
		ty->Sub(y0, y1);
		z2->Mul(x1, y1);

		Ref<BigInt> xi, yi, zi, zzi;
		xi.instantiate();
		xi->_set_abs(x);
		yi.instantiate();
		yi->_set_abs(y);
		zi.instantiate();
		zi->_set_abs(z);
		zzi.instantiate();
		zzi->_set_abs(zz);

		print_line("karatsuba wrong");
		print_line("x =", ifmt(xi));
		print_line("y =", ifmt(yi));
		print_line("z =", ifmt(zi));
		print_line("zz=", ifmt(zzi));
		print_line("x0=", ifmt(x0));
		print_line("x1=", ifmt(x1));
		print_line("y0=", ifmt(y0));
		print_line("y1=", ifmt(y1));
		print_line("tx=", ifmt(tx));
		print_line("ty=", ifmt(ty));
		print_line("z0=", ifmt(z0));
		print_line("z1=", ifmt(z1));
		print_line("z2=", ifmt(z2));
		CRASH_NOW_MSG("karatsuba");
	}
#endif
}

// karatsubaSqr squares x,
// writing the (non-normalized) result to z.
// z must have length 2*len(x).
// It is analogous to [karatsuba] but can run faster
// knowing both multiplicands are the same value.
void nat_karatsubaSqr(PackedInt64Array &z, PackedInt64Array x) {
	const int64_t n = x.size();
	CRASH_COND(z.size() != 2 * n);

	if (n < karatsubaSqrThreshold || n < 2) {
		nat_basicSqr(z, x);
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

	PackedInt64Array x0n = x.slice(0, n2);
	nat_norm(x0n);
	PackedInt64Array x1n = x.slice(n2);
	nat_norm(x1n);

	Ref<BigInt> x0, x1, z0, z1, z2, tx;
	x0.instantiate();
	x0->_set_abs(x0n);
	x1.instantiate();
	x1->_set_abs(x1n);
	z0.instantiate();
	z2.instantiate();

	// Allocate temporary storage for z1; repurpose z0 to hold tx.
	z1.instantiate();
	tx.instantiate();

	tx->Sub(x0, x1);
	PackedInt64Array z1n;
	nat_sqr(z1n, tx->_get_abs());
	z1->_set_abs(z1n);
	z1->_set_neg(true);

	PackedInt64Array z0n, z2n;
	nat_sqr(z0n, x0n);
	nat_sqr(z2n, x1n);
	z0->_set_abs(z0n);
	z2->_set_abs(z2n);
	z1->Add(z1, z0);
	z1->Add(z1, z2);
	z.fill(0);
	nat_copy(z, 0, z0n, 0, z0n.size());
	nat_copy(z, n2 * 2, z2n, 0, z2n.size());
	nat_addTo(z, n2, z1->_get_abs());

#ifdef GODOT_BIG_DEBUG_KARATSUBA
	// Debug mode: double-check answer and print trace on failure.
	PackedInt64Array zz;
	zz.resize(2 * n);
	nat_basicSqr(zz, x);
	if (nat_cmp(z, zz) != 0) {
		// All the temps were aliased to z and gone. Recompute.
		tx->Sub(x0, x1);
		z0->Mul(x0, x0);
		z2->Mul(x1, x1);
		z1->Mul(tx, tx);
		z1->Neg(z1);
		z1->Add(z1, z0);
		z1->Add(z1, z2);

		Ref<BigInt> xi, zi, zzi;
		xi.instantiate();
		xi->_set_abs(x);
		zi.instantiate();
		zi->_set_abs(z);
		zzi.instantiate();
		zzi->_set_abs(zz);

		print_line("karatsubaSqr wrong");
		print_line("x =", ifmt(xi));
		print_line("z =", ifmt(zi));
		print_line("zz=", ifmt(zzi));
		print_line("x0=", ifmt(x0));
		print_line("x1=", ifmt(x1));
		print_line("z0=", ifmt(z0));
		print_line("z1=", ifmt(z1));
		print_line("z2=", ifmt(z2));
		CRASH_NOW_MSG("karatsubaSqr");
	}
#endif
}
