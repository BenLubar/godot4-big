// This file is ported from src/math/big/arith.go in Go 1.27.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file provides Go implementations of elementary multi-precision
// arithmetic operations on word vectors. These have the suffix _g.
// These are needed for platforms without assembly implementations of these routines.
// This file also contains elementary operations that can be implemented
// sufficiently efficiently in Go.

#include "godot_big_naturals.h"

#include <bit>

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

// z1<<_W + z0 = x*y
void BigNat::mulWW(BigWord p_x, BigWord p_y, BigWord &r_z1, BigWord &r_z0) noexcept {
	bits_Mul(p_x, p_y, r_z1, r_z0);
}

// z1<<_W + z0 = x*y + c
void BigNat::mulAddWWW(BigWord p_x, BigWord p_y, BigWord p_c, BigWord &r_z1, BigWord &r_z0) noexcept {
	BigWord hi;
	BigWord lo;
	BigWord cc;
	bits_Mul(p_x, p_y, hi, lo);
	bits_Add(lo, p_c, 0, r_z0, cc);
	r_z1 = hi + cc;
}

// nlz returns the number of leading zeros in x.
// Wraps bits.LeadingZeros call for convenience.
int BigNat::nlz(BigWord p_x) noexcept {
	return std::countl_zero(p_x);
}

// The resulting carry c is either 0 or 1.
BigWord BigNat::addVV(Slice r_z, const CSlice &p_x, const CSlice &p_y) {
	CRASH_COND_MSG(p_x.size() != r_z.size() || p_y.size() != r_z.size(), "addVV len");

	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();

	BigWord c = 0;
	for (int64_t i = 0; i < r_z.length; i++) {
		bits_Add(x[i], y[i], c, r_z.array[i], c);
	}

	return c;
}

// The resulting carry c is either 0 or 1.
BigWord BigNat::subVV(Slice r_z, const CSlice &p_x, const CSlice &p_y) {
	CRASH_COND_MSG(p_x.size() != r_z.size() || p_y.size() != r_z.size(), "subVV len");

	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();

	BigWord c = 0;
	for (int64_t i = 0; i < r_z.length; i++) {
		bits_Sub(x[i], y[i], c, r_z.array[i], c);
	}

	return c;
}

// addVW sets z = x + y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
BigWord BigNat::addVW(Slice r_z, const CSlice &p_x, BigWord p_y) {
	CRASH_COND_MSG(p_x.size() != r_z.size(), "addVW len");

	if (r_z.length == 0) {
		return p_y;
	}

	const BigWord *x = p_x.ptr();

	BigWord c;
	bits_Add(x[0], p_y, 0, r_z.array[0], c);
	if (likely(c == 0)) {
		if (r_z.array != x) {
			memcpy(r_z.array + 1, x + 1, (r_z.size() - 1) * sizeof(BigWord));
		}
		return 0;
	}

	for (uint64_t i = 1; i < r_z.size(); i++) {
		if (likely(x[i] != UINT64_MAX)) {
			r_z.array[i] = x[i] + 1;
			if (r_z.array != x) {
				memcpy(r_z.array + i + 1, x + i + 1, (r_z.size() - i - 1) * sizeof(BigWord));
			}
			return 0;
		}

		r_z.array[i] = 0;
	}

	return 1;
}

// addVW_ref is the reference implementation for addVW, used only for testing.
BigWord BigNat::addVW_ref(Slice r_z, const CSlice &p_x, BigWord p_y) {
	BigWord c = p_y;
	for (uint64_t i = 0; i < r_z.size(); i++) {
		bits_Add(p_x[i], c, 0, r_z[i], c);
	}

	return c;
}

// subVW sets z = x - y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
BigWord BigNat::subVW(Slice r_z, const CSlice &p_x, BigWord p_y) {
	CRASH_COND_MSG(p_x.size() != r_z.size(), "subVW len");

	if (r_z.length == 0) {
		return p_y;
	}

	const BigWord *x = p_x.ptr();

	BigWord c;
	bits_Sub(x[0], p_y, 0, r_z.array[0], c);
	if (likely(c == 0)) {
		if (r_z.array != x) {
			memcpy(r_z.array + 1, x + 1, (r_z.size() - 1) * sizeof(BigWord));
		}
		return 0;
	}

	for (uint64_t i = 1; i < r_z.size(); i++) {
		if (likely(x[i] != 0)) {
			r_z.array[i] = x[i] - 1;
			if (r_z.array != x) {
				memcpy(r_z.array + i + 1, x + i + 1, (r_z.size() - i - 1) * sizeof(BigWord));
			}
			return 0;
		}

		r_z.array[i] = UINT64_MAX;
	}

	return 1;
}

// subVW_ref is the reference implementation for subVW, used only for testing.
BigWord BigNat::subVW_ref(Slice r_z, const CSlice &p_x, BigWord p_y) {
	BigWord c = p_y;
	for (uint64_t i = 0; i < r_z.size(); i++) {
		bits_Sub(p_x[i], c, 0, r_z[i], c);
	}

	return c;
}

BigWord BigNat::lshVU(Slice r_z, const CSlice &p_x, uint64_t p_s) {
	CRASH_COND_MSG(p_x.size() != r_z.size(), "lshVU len");

	if (p_s == 0) {
		if (r_z.array != p_x.ptr()) {
			memcpy(r_z.array, p_x.ptr(), r_z.size() * sizeof(BigWord));
		}
		return 0;
	}

	if (r_z.length == 0) {
		return 0;
	}

	const BigWord *x = p_x.ptr();

	p_s &= UINT64_C(63); // hint to the compiler that shifts by s don't need guard code
	const uint64_t s_hat = (UINT64_C(64) - p_s) & UINT64_C(63); // ditto

	const BigWord c = x[r_z.length - 1] >> s_hat;

	for (uint64_t i = r_z.size() - 1; i > 0; i--) {
		r_z.array[i] = (x[i] << p_s) | (x[i - 1] >> s_hat);
	}
	r_z.array[0] = x[0] << p_s;

	return c;
}

BigWord BigNat::rshVU(Slice r_z, const CSlice &p_x, uint64_t p_s) {
	CRASH_COND_MSG(p_x.size() != r_z.size(), "rshVU len");

	if (p_s == 0) {
		if (r_z.array != p_x.ptr()) {
			memcpy(r_z.array, p_x.ptr(), r_z.size() * sizeof(BigWord));
		}
		return 0;
	}

	if (r_z.length == 0) {
		return 0;
	}

	const BigWord *x = p_x.ptr();

	p_s &= UINT64_C(63); // hint to the compiler that shifts by s don't need guard code
	const uint64_t s_hat = (UINT64_C(64) - p_s) & UINT64_C(63); // ditto

	const BigWord c = x[0] << s_hat;

	for (uint64_t i = 1; i < r_z.size(); i++) {
		r_z.array[i - 1] = (x[i - 1] >> p_s) | (x[i] << s_hat);
	}
	r_z.array[r_z.length - 1] = x[r_z.length - 1] >> p_s;

	return c;
}

BigWord BigNat::mulAddVWW(Slice r_z, const CSlice &p_x, BigWord p_y, BigWord p_r) {
	CRASH_COND_MSG(p_x.size() != r_z.size(), "mulAddVWW len");

	const BigWord *x = p_x.ptr();

	BigWord c = p_r;
	for (uint64_t i = 0; i < r_z.size(); i++) {
		mulAddWWW(x[i], p_y, c, c, r_z.array[i]);
	}

	return c;
}

BigWord BigNat::addMulVVWW(Slice r_z, const CSlice &p_x, const CSlice &p_y, BigWord p_m, BigWord p_a) {
	CRASH_COND_MSG(p_x.size() != r_z.size() || p_y.size() != r_z.size(), "addMulVVWW len");

	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();

	BigWord c = p_a;
	BigWord z1;
	BigWord z0;
	for (uint64_t i = 0; i < r_z.size(); i++) {
		mulAddWWW(y[i], p_m, x[i], z1, z0);
		bits_Add(z0, c, 0, r_z.array[i], c);
		c += z1;
	}

	return c;
}

// q = ( x1 << _W + x0 - r)/y. m = floor(( _B^2 - 1 ) / d - _B). Requiring x1<y.
// An approximate reciprocal with a reference to "Improved Division by Invariant Integers
// (IEEE Transactions on Computers, 11 Jun. 2010)"
void BigNat::divWW(BigWord p_x1, BigWord p_x0, BigWord p_y, BigWord p_m, BigWord &r_q, BigWord &r_r) {
	const int s = nlz(p_y);
	if (s != 0) {
		p_x1 = (p_x1 << s) | (p_x0 >> (64 - s));
		p_x0 <<= s;
		p_y <<= s;
	}

	BigWord d = p_y;
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
	BigWord t1;
	BigWord t0;
	BigWord c;
	[[maybe_unused]] BigWord discard;
	bits_Mul(p_m, p_x1, t1, t0); // NOLINT(readability-suspicious-call-argument)
	bits_Add(t0, p_x0, 0, discard, c);
	bits_Add(t1, p_x1, c, t1, discard);

	// The quotient is either t1, t1+1, or t1+2.
	// We'll try t1 and adjust if needed.
	BigWord qq = t1;

	// compute remainder r=x-d*q.
	BigWord dq1;
	BigWord dq0;
	bits_Mul(d, qq, dq1, dq0);

	BigWord r1;
	BigWord r0;
	BigWord b;
	bits_Sub(p_x0, dq0, 0, r0, b);
	bits_Sub(p_x1, dq1, b, r1, discard);

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

	r_q = qq;
	r_r = r0 >> s;
}

// reciprocalWord return the reciprocal of the divisor. rec = floor(( _B^2 - 1 ) / u - _B). u = d1 << nlz(d1).
BigWord BigNat::reciprocalWord(BigWord p_d1) {
	const BigWord u = p_d1 << nlz(p_d1);
	const BigWord x1 = ~u;
	const BigWord x0 = UINT64_MAX;
	BigWord rec;
	BigWord discard;
	bits_Div(x1, x0, u, rec, discard); // (_B^2-1)/U-_B = (_B*(_M-C)+_M)/U
	return rec;
}
