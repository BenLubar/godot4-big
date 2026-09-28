// This file is ported from src/math/big/arith.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"

#include <bit>

using namespace godot;

// This file provides Go implementations of elementary multi-precision
// arithmetic operations on word vectors. These have the suffix _g.
// These are needed for platforms without assembly implementations of these routines.
// This file also contains elementary operations that can be implemented
// sufficiently efficiently in Go.

// In these routines, it is the caller's responsibility to arrange for
// x, y, and z to all have the same length. We check this and panic.
// The assembly versions of these routines do not include that check.
//
// The check+panic also has the effect of teaching the compiler that
// “i in range for z” implies “i in range for x and y”, eliminating all
// bounds checks in loops from 0 to len(z) and vice versa.

// ----------------------------------------------------------------------------
// Elementary operations on words
//
// These operations are used by the vector operations below.

// z1<<_W + z0 = x*y + c
void BigNat::mulAddWWW(BigWord p_x, BigWord p_y, BigWord p_c, BigWord &r_z1, BigWord &r_z0) {
	BigWord carry;
	bits_Mul(p_x, p_y, r_z1, carry);
	bits_Add(carry, p_c, 0, r_z0, carry);
	r_z1 += carry;
}

// The resulting carry c is either 0 or 1.
BigWord BigNat::addVV(BigNat &r_z, BigNat p_x, BigNat p_y) {
	CRASH_COND(p_x.array.size() != r_z.array.size() || p_y.array.size() != r_z.array.size());

	BigWord c = 0;
	for (int64_t i = 0; i < r_z.array.size(); i++) {
		bits_Add(p_x[i], p_y[i], c, r_z[i], c);
	}

	return c;
}

// The resulting carry c is either 0 or 1.
BigWord BigNat::subVV(BigNat &r_z, BigNat p_x, BigNat p_y) {
	CRASH_COND(p_x.array.size() != r_z.array.size() || p_y.array.size() != r_z.array.size());

	BigWord c = 0;
	for (int64_t i = 0; i < r_z.array.size(); i++) {
		bits_Sub(p_x[i], p_y[i], c, r_z[i], c);
	}

	return c;
}

// addVW sets z = x + y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
BigWord BigNat::addVW(BigNat &r_z, BigNat p_x, BigWord p_y) {
	CRASH_COND(p_x.array.size() != r_z.array.size());

	if (r_z.array.is_empty()) {
		return p_y;
	}

	BigWord c;
	bits_Add(p_x[0], p_y, 0, r_z[0], c);
	if (c == 0) {
		memmove(r_z.array.ptrw() + 1, p_x.array.ptr() + 1, (r_z.array.size() - 1) * 8);
		return 0;
	}

	for (int64_t i = 1; i < r_z.array.size(); i++) {
		const BigWord xi = p_x[i];
		if (xi != UINT64_MAX) {
			r_z[i] = xi + 1;
			memmove(r_z.array.ptrw() + i + 1, p_x.array.ptr() + i + 1, (r_z.array.size() - i - 1) * 8);
			return 0;
		}

		r_z[i] = 0;
	}

	return 1;
}

// subVW sets z = x - y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
BigWord BigNat::subVW(BigNat &r_z, BigNat p_x, BigWord p_y) {
	CRASH_COND(p_x.array.size() != r_z.array.size());

	if (r_z.array.is_empty()) {
		return p_y;
	}

	BigWord c;
	bits_Sub(p_x[0], p_y, 0, r_z[0], c);
	if (c == 0) {
		memmove(r_z.array.ptrw() + 1, p_x.array.ptr() + 1, (r_z.array.size() - 1) * 8);
		return 0;
	}

	for (int64_t i = 1; i < r_z.array.size(); i++) {
		const BigWord xi = p_x[i];
		if (xi != 0) {
			r_z[i] = xi - 1;
			memmove(r_z.array.ptrw() + i + 1, p_x.array.ptr() + i + 1, (r_z.array.size() - i - 1) * 8);
			return 0;
		}

		r_z[i] = UINT64_MAX;
	}

	return 1;
}

BigWord BigNat::lshVU(BigNat &r_z, BigNat p_x, uint64_t p_s) {
	CRASH_COND(r_z.array.size() != p_x.array.size());

	if (p_s == 0) {
		r_z.array = p_x.array;
		return 0;
	}

	if (r_z.array.is_empty()) {
		return 0;
	}

	p_s &= 64 - 1; // hint to the compiler that shifts by s don't need guard code
	uint64_t shat = 64 - p_s;
	shat &= 64 - 1; // ditto

	const BigWord c = p_x[r_z.array.size() - 1] >> shat;
	for (int64_t i = r_z.array.size() - 1; i > 0; i--) {
		r_z[i] = (p_x[i] << p_s) | (p_x[i - 1] >> shat);
	}

	r_z[0] = p_x[0] << p_s;

	return c;
}

BigWord BigNat::rshVU(BigNat &r_z, BigNat p_x, uint64_t p_s) {
	CRASH_COND(r_z.array.size() != p_x.array.size());

	if (p_s == 0) {
		r_z.array = p_x.array;
		return 0;
	}

	if (r_z.array.is_empty()) {
		return 0;
	}

	p_s &= 64 - 1; // hint to the compiler that shifts by s don't need guard code
	uint64_t shat = 64 - p_s;
	shat &= 64 - 1; // ditto

	const BigWord c = p_x[0] << shat;

	for (int64_t i = 1; i < r_z.array.size(); i++) {
		r_z[i - 1] = (p_x[i - 1] >> p_s) | (p_x[i] << shat);
	}

	r_z[r_z.array.size() - 1] = p_x[r_z.array.size() - 1] >> p_s;

	return c;
}

BigWord BigNat::subVVSlice(BigNat &r_z, int64_t p_begin, int64_t p_end, BigNat p_y) {
	BigNat z{ r_z.array.slice(p_begin, p_end) };
	const BigWord c = subVV(z, z, p_y); // NOLINT(performance-unnecessary-value-param)
	memmove(r_z.array.ptrw() + p_begin, z.array.ptr(), (p_end - p_begin) * sizeof(BigWord));
	return c;
}

BigWord BigNat::subVWSlice(BigNat &r_z, int64_t p_begin, int64_t p_end, BigWord p_y) {
	BigNat z{ r_z.array.slice(p_begin, p_end) };
	const BigWord c = subVW(z, z, p_y);
	memmove(r_z.array.ptrw() + p_begin, z.array.ptr(), (p_end - p_begin) * sizeof(BigWord));
	return c;
}

BigWord BigNat::mulAddVWW(BigNat &r_z, BigNat p_x, BigWord p_y, BigWord p_r) {
	CRASH_COND(p_x.array.size() != r_z.array.size());

	BigWord c = p_r;
	for (int64_t i = 0; i < r_z.array.size(); i++) {
		mulAddWWW(p_x[i], p_y, c, c, r_z[i]);
	}
	return c;
}

BigWord BigNat::addMulVVWW(BigNat &r_z, BigNat p_x, BigNat p_y, BigWord p_m, BigWord p_a) {
	CRASH_COND(p_x.array.size() != r_z.array.size() || p_x.array.size() != r_z.array.size());

	BigWord c = p_a;
	for (int64_t i = 0; i < r_z.array.size(); i++) {
		BigWord z1;
		BigWord z0;
		mulAddWWW(p_y[i], p_m, p_x[i], z1, z0);
		bits_Add(z0, c, 0, r_z[i], c);
		c += z1;
	}
	return c;
}

// q = ( x1 << _W + x0 - r)/y. m = floor(( _B^2 - 1 ) / d - _B). Requiring x1<y.
// An approximate reciprocal with a reference to "Improved Division by Invariant Integers
// (IEEE Transactions on Computers, 11 Jun. 2010)"
void BigNat::divWW(BigWord x1, BigWord x0, BigWord y, BigWord m, BigWord &q, BigWord &r) {
	const uint64_t s = std::countl_zero(y);
	if (s != 0) {
		x1 = (x1 << s) | (x0 >> (64 - s));
		x0 <<= s;
		y <<= s;
	}

	uint64_t d = y;

	// We know that
	//   m = ⎣(B^2-1)/d⎦-B
	//   ⎣(B^2-1)/d⎦ = m+B
	//   (B^2-1)/d = m+B+delta1    0 <= delta1 <= (d-1)/d
	//   B^2/d = m+B+delta2        0 <= delta2 <= 1
	// The quotient we're trying to compute is
	//   quotient = ⎣(x1*B+x0)/d⎦
	//            = ⎣(x1*B*(B^2/d)+x0*(B^2/d))/B^2⎦
	//            = ⎣(x1*B*(m+B+delta2)+x0*(m+B+delta2))/B^2⎦
	//            = ⎣(x1*m+x1*B+x0)/B + x0*m/B^2 + delta2*(x1*B+x0)/B^2⎦
	// The latter two terms of this three-term sum are between 0 and 1.
	// So we can compute just the first term, and we will be low by at most 2.
	uint64_t t1;
	uint64_t t0;
	uint64_t c;
	uint64_t discard;
	bits_Mul(m, x1, t1, t0);
	bits_Add(t0, x0, 0, discard, c);
	bits_Add(t1, x1, c, t1, discard);

	// The quotient is either t1, t1+1, or t1+2.
	// We'll try t1 and adjust if needed.
	uint64_t qq = t1;

	// compute remainder r=x-d*q.
	uint64_t dq1;
	uint64_t dq0;
	uint64_t r0;
	uint64_t r1;
	uint64_t b;
	bits_Mul(d, qq, dq1, dq0);
	bits_Sub(x0, dq0, 0, r0, b);
	bits_Sub(x1, dq1, b, r1, discard);

	// The remainder we just computed is bounded above by B+d:
	// r = x1*B + x0 - d*q.
	//   = x1*B + x0 - d*⎣(x1*m+x1*B+x0)/B⎦
	//   = x1*B + x0 - d*((x1*m+x1*B+x0)/B-alpha)                                   0 <= alpha < 1
	//   = x1*B + x0 - x1*d/B*m                         - x1*d - x0*d/B + d*alpha
	//   = x1*B + x0 - x1*d/B*⎣(B^2-1)/d-B⎦             - x1*d - x0*d/B + d*alpha
	//   = x1*B + x0 - x1*d/B*⎣(B^2-1)/d-B⎦             - x1*d - x0*d/B + d*alpha
	//   = x1*B + x0 - x1*d/B*((B^2-1)/d-B-beta)        - x1*d - x0*d/B + d*alpha   0 <= beta < 1
	//   = x1*B + x0 - x1*B + x1/B + x1*d + x1*d/B*beta - x1*d - x0*d/B + d*alpha
	//   =        x0        + x1/B        + x1*d/B*beta        - x0*d/B + d*alpha
	//   = x0*(1-d/B) + x1*(1+d*beta)/B + d*alpha
	//   <  B*(1-d/B) +  d*B/B          + d          because x0<B (and 1-d/B>0), x1<d, 1+d*beta<=B, alpha<1
	//   =  B - d     +  d              + d
	//   = B+d
	// So r1 can only be 0 or 1. If r1 is 1, then we know q was too small.
	// Add 1 to q and subtract d from r. That guarantees that r is <B, so
	// we no longer need to keep track of r1.
	if (r1 != 0) {
		qq++;
		r0 -= d;
	}

	// If the remainder is still too large, increment q one more time.
	if (r0 >= d) {
		qq++;
		r0 -= d;
	}

	q = qq;
	r = r0 >> s;
}

// reciprocalWord return the reciprocal of the divisor. rec = floor(( _B^2 - 1 ) / u - _B). u = d1 << nlz(d1).
BigWord BigNat::reciprocalWord(BigWord d1) {
	const uint64_t u = d1 << std::countl_zero(d1);
	const uint64_t x1 = ~u;
	const uint64_t x0 = UINT64_MAX;

	uint64_t q;
	uint64_t r;
	bits_Div(x1, x0, u, q, r); // (_B^2-1)/U-_B = (_B*(_M-C)+_M)/U
	return q;
}
