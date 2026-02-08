// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"

// This file provides Go implementations of elementary multi-precision
// arithmetic operations on word vectors. These have the suffix _g.
// These are needed for platforms without assembly implementations of these routines.
// This file also contains elementary operations that can be implemented
// sufficiently efficiently in Go.

/*

// A Word represents a single digit of a multi-precision unsigned integer.
type Word uint

const (
	_S = _W / 8 // word size in bytes

	_W = bits.UintSize // word size in bits
	_B = 1 << _W       // digit base
	_M = _B - 1        // digit mask
)
*/

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
void nat_mulWW(uint64_t x, uint64_t y, uint64_t &z1, uint64_t &z0) {
	static constexpr uint64_t mask32 = (1LLU << 32) - 1;
	uint64_t x0 = x & mask32;
	uint64_t x1 = x >> 32;
	uint64_t y0 = y & mask32;
	uint64_t y1 = y >> 32;
	uint64_t w0 = x0 * y0;
	uint64_t t = x1 * y0 + (w0 >> 32);
	uint64_t w1 = t & mask32;
	uint64_t w2 = t >> 32;
	w1 += x0 * y1;
	z1 = x1 * y1 + w2 + (w1 >> 32);
	z0 = x * y;
}

// z1<<_W + z0 = x*y + c
void nat_mulAddWWW(uint64_t x, uint64_t y, uint64_t c, uint64_t &z1, uint64_t &z0) {
	uint64_t hi, lo;
	nat_mulWW(x, y, hi, lo);
	uint64_t cc = 0;
	lo = nat_addWW(lo, c, cc);
	z1 = hi + cc;
	z0 = lo;
}

// nlz returns the number of leading zeros in x.
// Wraps bits.LeadingZeros call for convenience.
uint64_t nat_nlz(uint64_t x) {
	return uint64_t(std::countl_zero(x));
}

// Add64 returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
uint64_t nat_addWW(uint64_t x, uint64_t y, uint64_t &carry) {
	const uint64_t sum = x + y + carry;

	// The sum will overflow if both top bits are set (x & y) or if one of them
	// is (x | y), and a carry from the lower place happened. If such a carry
	// happens, the top bit will be 1 + 0 + 1 = 0 (&^ sum).
	carry = ((x & y) | ((x | y) & ~sum)) >> 63;

	return sum;
}

// The resulting carry c is either 0 or 1.
uint64_t nat_addVV(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, int64_t n) {
	uint64_t c = 0;

	for (int64_t i = 0; i < n; i++) {
		z[i + zoff] = nat_addWW(uint64_t(x[i + xoff]), uint64_t(y[i + yoff]), c);
	}

	return c;
}

// Sub64 returns the difference of x, y and borrow: diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
uint64_t nat_subWW(uint64_t x, uint64_t y, uint64_t &borrow) {
	const uint64_t diff = x - y - borrow;

	// The difference will underflow if the top bit of x is not set and the top
	// bit of y is set (^x & y) or if they are the same (^(x ^ y)) and a borrow
	// from the lower place happens. If that borrow happens, the result will be
	// 1 - 1 - 1 = 0 - 0 - 1 = 1 (& diff).
	borrow = (((~x) & y) | ((~(x ^ y)) & diff)) >> 63;

	return diff;
}

// The resulting carry c is either 0 or 1.
uint64_t nat_subVV(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, int64_t n) {
	uint64_t c = 0;
	for (int64_t i = 0; i < n; i++) {
		z[i + zoff] = nat_subWW(uint64_t(x[i + xoff]), uint64_t(y[i + yoff]), c);
	}

	return c;
}

// addVW sets z = x + y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
uint64_t nat_addVW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, int64_t n) {
	if (n == 0) {
		return y;
	}

	uint64_t cc = 0;
	z[zoff] = nat_addWW(uint64_t(x[xoff]), y, cc);

	if (cc == 0) {
		nat_copy(z, 1 + zoff, x, 1 + xoff, n - 1);
		return 0;
	}

	for (int64_t i = 1; i < n; i++) {
		uint64_t xi = uint64_t(x[i + xoff]);
		if (~xi != 0) {
			z[i + zoff] = xi + 1;
			nat_copy(z, i + 1 + zoff, x, i + 1 + xoff, n - i - 1);

			return 0;
		}

		z[i + zoff] = 0;
	}

	return 1;
}

// subVW sets z = x - y, returning the final carry c.
// The behavior is undefined if len(x) != len(z).
// If len(z) == 0, c = y; otherwise, c is 0 or 1.
uint64_t nat_subVW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, int64_t n) {
	if (n == 0) {
		return y;
	}

	uint64_t cc = 0;
	z[zoff] = nat_subWW(uint64_t(x[xoff]), y, cc);
	if (cc == 0) {
		nat_copy(z, zoff + 1, x, xoff + 1, n - 1);
		return 0;
	}

	for (int64_t i = 1; i < n; i++) {
		const uint64_t xi = x[i + xoff];
		if (xi != 0) {
			z[i + zoff] = xi - 1;
			nat_copy(z, i + 1 + zoff, x, i + 1 + xoff, n - i - 1);

			return 0;
		}

		z[i + zoff] = ~uint64_t(0);
	}

	return 1;
}

uint64_t nat_lshVU(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t s, int64_t n) {
	if (s == 0) {
		nat_copy(z, zoff, x, xoff, n);
		return 0;
	}

	if (n == 0) {
		return 0;
	}

	const uint64_t c = uint64_t(x[n - 1 + xoff]) >> (64 - s);
	for (int64_t i = n - 1; i > 0; i--) {
		z[i + zoff] = (uint64_t(x[i + xoff]) << s) | (uint64_t(x[i - 1 + xoff]) >> (64 - s));
	}

	z[zoff] = uint64_t(x[xoff]) << s;

	return c;
}

uint64_t nat_rshVU(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t s, int64_t n) {
	if (s == 0) {
		nat_copy(z, zoff, x, xoff, n);
		return 0;
	}

	if (n == 0) {
		return 0;
	}

	const uint64_t c = uint64_t(x[0]) << (64 - s);
	for (int64_t i = 1; i < n; i++) {
		z[i - 1 + zoff] = (uint64_t(x[i - 1 + xoff]) >> s) | (uint64_t(x[i + xoff]) << (64 - s));
	}
	z[n - 1 + zoff] = uint64_t(x[n - 1 + xoff]) >> s;

	return c;
}

uint64_t nat_mulAddVWW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, uint64_t r, int64_t n) {
	uint64_t c = r;
	uint64_t zi;
	for (int64_t i = 0; i < n; i++) {
		nat_mulAddWWW(x[i + xoff], y, c, c, zi);
		z[i + zoff] = zi;
	}

	return c;
}

uint64_t nat_addMulVVWW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, uint64_t m, uint64_t a, int64_t n) {
	uint64_t c = a;
	for (int64_t i = 0; i < n; i++) {
		uint64_t z1, z0;
		nat_mulAddWWW(y[i + yoff], m, x[i + xoff], z1, z0);
		uint64_t cc = 0;
		z[i + zoff] = nat_addWW(z0, c, cc);
		c = cc + z1;
	}

	return c;
}

// q = ( x1 << _W + x0 - r)/y. m = floor(( _B^2 - 1 ) / d - _B). Requiring x1<y.
// An approximate reciprocal with a reference to "Improved Division by Invariant Integers
// (IEEE Transactions on Computers, 11 Jun. 2010)"
void nat_divWW(uint64_t x1, uint64_t x0, uint64_t y, uint64_t m, uint64_t &q, uint64_t &r) {
	const uint64_t s = nat_nlz(y);
	if (s != 0) {
		x1 = (x1 << s) | (x0 >> (64 - s));
		x0 <<= s;
		y <<= s;
	}

	uint64_t d = y, c = 0;
	uint64_t t1, t0;

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
	nat_mulWW(m, x1, t1, t0);
	nat_addWW(t0, x0, c);
	t1 = nat_addWW(t1, x1, c);

	// The quotient is either t1, t1+1, or t1+2.
	// We'll try t1 and adjust if needed.
	uint64_t qq = t1;

	// compute remainder r=x-d*q.
	uint64_t dq1, dq0;
	uint64_t b = 0;
	nat_mulWW(d, qq, dq1, dq0);
	uint64_t r0 = nat_subWW(x0, dq0, b);
	uint64_t r1 = nat_subWW(x1, dq1, b);

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

// Div64 returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div64 panics for y == 0 (division by zero) or y <= hi (quotient overflow).
void nat_div64(uint64_t hi, uint64_t lo, uint64_t y, uint64_t &quo, uint64_t &rem) {
	CRASH_COND_MSG(y == 0, "division by zero");
	CRASH_COND_MSG(y <= hi, "quotient overflow");

	// If high part is zero, we can directly return the results.
	if (hi == 0) {
		quo = lo / y;
		rem = lo % y;
		return;
	}

	const uint64_t s = std::countl_zero(y);
	y <<= s;

	static constexpr uint64_t two32 = 1LLU << 32;
	static constexpr uint64_t mask32 = two32 - 1;

	const uint64_t yn1 = y >> 32;
	const uint64_t yn0 = y & mask32;
	const uint64_t un32 = (hi << s) | (lo >> (64 - s));
	const uint64_t un10 = lo << s;
	const uint64_t un1 = un10 >> 32;
	const uint64_t un0 = un10 & mask32;
	uint64_t q1 = un32 / yn1;
	uint64_t rhat = un32 - q1 * yn1;

	while (q1 >= two32 || q1 * yn0 > two32 * rhat + un1) {
		q1--;
		rhat += yn1;
		if (rhat >= two32) {
			break;
		}
	}

	const uint64_t un21 = un32 * two32 + un1 - q1 * y;
	uint64_t q0 = un21 / yn1;
	rhat = un21 - q0 * yn1;

	while (q0 >= two32 || q0 * yn0 > two32 * rhat + un0) {
		q0--;
		rhat += yn1;
		if (rhat >= two32) {
			break;
		}
	}

	quo = q1 * two32 + q0;
	rem = (un21 * two32 + un0 - q0 * y) >> s;
}

// reciprocalWord return the reciprocal of the divisor. rec = floor(( _B^2 - 1 ) / u - _B). u = d1 << nlz(d1).
uint64_t nat_reciprocalWord(uint64_t d1) {
	uint64_t u = d1 << nat_nlz(d1);
	uint64_t x1 = ~u;
	uint64_t x0 = ~uint64_t(0);
	uint64_t rec, rem;
	nat_div64(x1, x0, u, rec, rem); // (_B^2-1)/U-_B = (_B*(_M-C)+_M)/U
	return rec;
}
