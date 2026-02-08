// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"

// This file implements unsigned multi-precision integers (natural
// numbers). They are the building blocks for the implementation
// of signed integers, rationals, and floating-point numbers.

// An unsigned integer x of the form
//
//	x = x[n-1]*_B^(n-1) + x[n-2]*_B^(n-2) + ... + x[1]*_B + x[0]
//
// with 0 <= x[i] < _B and 0 <= i < n is stored in a slice of length n,
// with the digits x[i] as the slice elements.
//
// A number is normalized if the slice contains no leading 0 digits.
// During arithmetic operations, denormalized values may occur but are
// always normalized before returning the final result. The normalized
// representation of 0 is the empty or nil slice (length = 0).

void nat_norm(PackedInt64Array &z) {
	int64_t i = z.size();
	while (i > 0 && z[i - 1] == 0) {
		i--;
	}

	z.resize(i);
}

void nat_make(PackedInt64Array &z, int64_t n) {
	// We don't have Go slices, so just let Godot handle the capacity.
	z.resize(n);
}

void nat_setWord(PackedInt64Array &z, uint64_t x) {
	if (x == 0) {
		z.clear();
	}

	z.resize(1);
	z[0] = x;
}

void nat_setUint64(PackedInt64Array &z, uint64_t x) {
	if (x == 0) {
		z.clear();
	}

	z.resize(1);
	z[0] = x;
}

void nat_set(PackedInt64Array &z, PackedInt64Array x) {
	z = x;
}

void nat_copy(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, int64_t n) {
	for (int64_t i = 0; i < n; i++) {
		z[i + zoff] = x[i + xoff];
	}
}

void nat_clear(PackedInt64Array &z, int64_t zoff, int64_t n) {
	for (int64_t i = 0; i < n; i++) {
		z[i + zoff] = 0;
	}
}

void nat_add(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();

	if (m < n) {
		std::swap(m, n);
		std::swap(x, y);
	}

	if (m == 0) {
		// n == 0 because m >= n; result is 0
		z.clear();
		return;
	}

	if (n == 0) {
		// result is x
		nat_set(z, x);
		return;
	}

	// m > 0

	nat_make(z, m + 1);
	uint64_t c = nat_addVV(z, 0, x, 0, y, 0, n);
	if (m > n) {
		c = nat_addVW(z, n, x, n, c, m - n);
	}
	z[m] = c;

	nat_norm(z);
}

void nat_sub(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();

	CRASH_COND_MSG(m < n, "underflow");

	if (m == 0) {
		// n == 0 because m >= n; result is 0
		z.clear();
		return;
	}

	if (n == 0) {
		// result is x
		nat_set(z, x);
		return;
	}

	// m > 0

	nat_make(z, m);
	uint64_t c = nat_subVV(z, 0, x, 0, y, 0, n);
	if (m > n) {
		c = nat_subVW(z, n, x, n, c, m - n);
	}
	CRASH_COND_MSG(c != 0, "underflow");

	nat_norm(z);
}

int nat_cmp(PackedInt64Array x, PackedInt64Array y) {
	const int64_t m = x.size();
	const int64_t n = y.size();
	if (m != n || m == 0) {
		if (m < n) {
			return -1;
		}

		if (m > n) {
			return 1;
		}

		return 0;
	}

	int64_t i = m - 1;
	while (i > 0 && x[i] == y[i]) {
		i--;
	}

	if (uint64_t(x[i]) < uint64_t(y[i])) {
		return -1;
	}

	if (uint64_t(x[i]) > uint64_t(y[i])) {
		return 1;
	}

	return 0;
}

// montgomery computes z mod m = x*y*2**(-n*_W) mod m,
// assuming k = -1/m mod 2**_W.
// z is used for storing the result which is returned;
// z must not alias x, y or m.
// See Gueron, "Efficient Software Implementations of Modular Exponentiation".
// https://eprint.iacr.org/2011/239.pdf
// In the terminology of that paper, this is an "Almost Montgomery Multiplication":
// x and y are required to satisfy 0 <= z < 2**(n*_W) and then the result
// z is guaranteed to satisfy 0 <= z < 2**(n*_W), but it may not be < m.
void nat_montgomery(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m, uint64_t k, uint64_t n) {
	// This code assumes x, y, m are all the same length, n.
	// (required by addMulVVW and the for loop).
	// It also assumes that x, y are already reduced mod m,
	// or else the result will not be properly reduced.
	CRASH_COND_MSG(x.size() != n || y.size() != n || m.size() != n, "math/big: mismatched montgomery number lengths");

	z.resize(n * 2);
	z.fill(0);

	uint64_t c = 0;
	for (int64_t i = 0; i < n; i++) {
		uint64_t d = y[i];
		uint64_t c2 = nat_addMulVVWW(z, i, z, i, x, 0, d, 0, n);
		uint64_t t = uint64_t(z[i]) * k;
		uint64_t c3 = nat_addMulVVWW(z, i, z, i, m, 0, t, 0, n);
		uint64_t cx = c + c2;
		uint64_t cy = cx + c3;
		z[n + i] = cy;
		if (cx < c2 || cy < c3) {
			c = 1;
		} else {
			c = 0;
		}
	}

	if (c != 0) {
		nat_subVV(z, 0, z, n, m, 0, n);
	} else {
		nat_copy(z, 0, z, n, n);
	}

	z.resize(n);
}

// addTo implements z += x; z must be long enough.
// (we don't use nat.add because we need z to stay the same
// slice, and we don't need to normalize z after each addition)
void nat_addTo(PackedInt64Array &z, int64_t zoff, PackedInt64Array x) {
	const int64_t n = x.size();
	if (n > 0) {
		uint64_t c = nat_addVV(z, zoff, z, zoff, x, 0, n);
		if (c != 0 && z.size() > zoff + n) {
			c = nat_addVW(z, zoff + n, z, zoff + n, c, z.size() - n - zoff);
		}
		CRASH_COND(c != 0);
	}
}

// mulRange computes the product of all the unsigned integers in the
// range [a, b] inclusively. If a > b (empty range), the result is 1.
// The caller may pass stk == nil to request that mulRange obtain and release one itself.
void nat_mulRange(PackedInt64Array &z, uint64_t a, uint64_t b) {
	if (a == 0) {
		// cut long ranges short (optimization)
		nat_setUint64(z, 0);
		return;
	}
	if (a > b) {
		nat_setUint64(z, 1);
		return;
	}
	if (a == b) {
		nat_setUint64(z, a);
		return;
	}

	PackedInt64Array na, nb;
	if (a + 1 == b) {
		nat_setUint64(na, a);
		nat_setUint64(nb, b);
		nat_mul(z, na, nb);
		return;
	}

	const uint64_t m = a + (b - a) / 2; // avoid overflow
	nat_mulRange(na, a, m);
	nat_mulRange(nb, m + 1, b);
	nat_mul(z, na, nb);
}

// bitLen returns the length of x in bits.
// Unlike most methods, it works even if x is not normalized.
int64_t nat_bitLen(PackedInt64Array x) {
	int64_t i = x.size() - 1;
	if (i >= 0) {
		return i * 64 + std::bit_width(uint64_t(x[i]));
	}

	return 0;
}

// trailingZeroBits returns the number of consecutive least significant zero
// bits of x.
uint64_t nat_trailingZeroBits(PackedInt64Array x) {
	if (x.is_empty()) {
		return 0;
	}

	uint64_t i = 0;
	while (x[i] == 0) {
		i++;
	}

	// x[i] != 0
	return i * 64 + uint64_t(std::countr_zero(uint64_t(x[i])));
}

// isPow2 returns i, true when x == 2**i and 0, false otherwise.
bool nat_isPow2(PackedInt64Array x, uint64_t &i) {
	i = 0;
	while (x[i] == 0) {
		i++;
	}

	if (i == uint64_t(x.size()) - 1 && (x[i] & (x[i] - 1)) == 0) {
		i = i * 64 + std::countr_zero(uint64_t(x[i]));
		return true;
	}

	i = 0;
	return false;
}

// z = x << s
void nat_lsh(PackedInt64Array &z, PackedInt64Array x, uint64_t s) {
	if (s == 0) {
		nat_set(z, x);
		return;
	}

	const int64_t m = x.size();
	if (m == 0) {
		z.clear();
		return;
	}
	// m > 0

	const int64_t n = m + int64_t(s / 64);
	z.resize(n + 1);
	s %= 64;
	if (s == 0) {
		nat_copy(z, n - m, x, 0, m);
		z[n] = 0;
	} else {
		z[n] = nat_lshVU(z, n - m, x, 0, s, m);
	}
	nat_clear(z, 0, n - m);

	nat_norm(z);
}

// z = x >> s
void nat_rsh(PackedInt64Array &z, PackedInt64Array x, uint64_t s) {
	if (s == 0) {
		nat_set(z, x);
		return;
	}

	const int64_t m = x.size();
	const int64_t n = m - int64_t(s / 64);
	if (n <= 0) {
		z.clear();
		return;
	}

	// n > 0

	z.resize(n);
	s %= 64;
	if (s == 0) {
		nat_copy(z, 0, x, m - n, n);
	} else {
		nat_rshVU(z, 0, x, m - n, s, n);
	}

	nat_norm(z);
}

void nat_setBit(PackedInt64Array &z, PackedInt64Array x, uint64_t i, uint64_t b) {
	int64_t j = i / 64;
	uint64_t m = uint64_t(1) << (i % 64);
	int64_t n = x.size();
	switch (b) {
	case 0:
		z.resize(n);
		nat_copy(z, 0, x, 0, n);
		if (j >= n) {
			// no need to grow
			return;
		}
		z[j] &= ~m;
		nat_norm(z);
		return;
	case 1:
		if (j >= n) {
			z.resize(j + 1);
			nat_clear(z, n, n - z.size());
		} else {
			z.resize(n);
		}
		nat_copy(z, 0, x, 0, n);
		z[j] |= m;
		// no need to normalize
		return;
	}

	CRASH_NOW_MSG("set bit is not 0 or 1");
}

// bit returns the value of the i'th bit, with lsb == bit 0.
uint64_t nat_bit(PackedInt64Array x, uint64_t i) {
	uint64_t j = i / 64;
	if (j >= uint64_t(x.size())) {
		return 0;
	}

	// 0 <= j < len(x)
	return uint64_t((x[j] >> (i % 64)) & 1);
}

// sticky returns 1 if there's a 1 bit within the
// i least significant bits, otherwise it returns 0.
uint64_t nat_sticky(PackedInt64Array x, uint64_t i) {
	uint64_t j = i / 64;
	if (j >= x.size()) {
		return x.is_empty() ? 0 : 1;
	}

	// 0 <= j < len(x)
	for (uint64_t k = 0; k < j; k++) {
		if (x[k] != 0) {
			return 1;
		}
	}

	return (x[j] << (64 - (i % 64))) != 0 ? 1 : 0;
}

void nat_and(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();
	if (m > n) {
		m = n;
	}
	// m <= n

	z.resize(m);
	for (int64_t i = 0; i < m; i++) {
		z[i] = x[i] & y[i];
	}

	nat_norm(z);
}

// trunc returns z = x mod 2ⁿ.
void nat_trunc(PackedInt64Array &z, PackedInt64Array x, uint64_t n) {
	z = x;

	uint64_t w = (n + 64 - 1) / 64;
	if (uint64_t(x.size()) < w) {
		return;
	}

	z.resize(w);

	if (n % 64 != 0) {
		z[z.size() - 1] &= (1LLU << (n % 64)) - 1;
	}

	nat_norm(z);
}

void nat_andNot(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();
	if (n > m) {
		n = m;
	}
	// m >= n

	nat_make(z, m);
	for (int64_t i = 0; i < n; i++) {
		z[i] = x[i] & ~y[i];
	}
	nat_copy(z, n, x, n, m - n);

	nat_norm(z);
}

void nat_or(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();
	PackedInt64Array s = x;
	if (m < n) {
		std::swap(m, n);
		s = y;
	}
	// m >= n

	z.resize(m);
	for (int64_t i = 0; i < n; i++) {
		z[i] = x[i] | y[i];
	}
	nat_copy(z, n, s, n, m - n);

	nat_norm(z);
}

void nat_xor(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	int64_t m = x.size();
	int64_t n = y.size();

	PackedInt64Array s = x;
	if (m < n) {
		std::swap(m, n);
		s = y;
	}

	// m >= n

	z.resize(m);
	for (int64_t i = 0; i < n; i++) {
		z[i] = uint64_t(x[i]) ^ uint64_t(y[i]);
	}
	nat_copy(z, n, s, n, m - n);

	nat_norm(z);
}

// random creates a random integer in [0..limit), using the space in z if
// possible. n is the bit length of limit.
void nat_random(PackedInt64Array &z, const Callable &rand, PackedInt64Array limit, int64_t n) {
	// rand is a function that takes no arguments and returns a random integer between 0 and 2^32 - 1.

	z.resize(limit.size());

	uint64_t bitLengthOfMSW = uint64_t(n % 64);
	if (bitLengthOfMSW == 0) {
		bitLengthOfMSW = 64;
	}

	const uint64_t mask = uint64_t(1LLU << bitLengthOfMSW) - 1;

	while (true) {
		for (int64_t i = 0; i < z.size(); i++) {
			z[i] = rand.call().operator uint64_t() | (rand.call().operator uint64_t() << 32);
		}

		z[limit.size() - 1] &= mask;
		if (nat_cmp(z, limit) < 0) {
			break;
		}
	}

	nat_norm(z);
}

// If m != 0 (i.e., len(m) != 0), expNN sets z to x**y mod m;
// otherwise it sets z to x**y. The result is the value of z.
// The caller may pass stk == nil to request that expNN obtain and release one itself.
void nat_expNN(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m, bool slow) {
	// x**y mod 1 == 0
	if (m.size() == 1 && m[0] == 1) {
		nat_setWord(z, 0);
		return;
	}
	// m == 0 || m > 1

	// x**0 == 1
	if (y.is_empty()) {
		nat_setWord(z, 1);
		return;
	}
	// y > 0

	// 0**y = 0
	if (x.is_empty()) {
		nat_setWord(z, 0);
		return;
	}
	// x > 0

	// 1**y = 1
	if (x.size() == 1 && x[0] == 1) {
		nat_setWord(z, 1);
		return;
	}
	// x > 1

	// x**1 == x
	if (y.size() == 1 && y[0] == 1 && m.is_empty()) {
		nat_set(z, x);
		return;
	}

	if (y.size() == 1 && y[0] == 1) { // len(m) > 0
		nat_rem(x, m, z);
		return;
	}

	// y > 1

	if (!m.is_empty()) {
		// We likely end up being as long as the modulus.
		z.resize(m.size());

		// If the exponent is large, we use the Montgomery method for odd values,
		// and a 4-bit, windowed exponentiation for powers of two,
		// and a CRT-decomposed Montgomery method for the remaining values
		// (even values times non-trivial odd values, which decompose into one
		// instance of each of the first two cases).
		if (y.size() > 1 && !slow) {
			if ((m[0] & 1) == 1) {
				nat_expNNMontgomery(z, x, y, m);
				return;
			}

			uint64_t logM;
			if (nat_isPow2(m, logM)) {
				nat_expNNWindowed(z, x, y, logM);
				return;
			}

			nat_expNNMontgomeryEven(z, x, y, m);
			return;
		}
	}

	nat_set(z, x);
	uint64_t v = y[y.size() - 1]; // v > 0 because y is normalized and y > 0
	uint64_t shift = nat_nlz(v) + 1;
	v <<= shift;
	PackedInt64Array q;

	static constexpr uint64_t mask = 1LLU << (64 - 1);

	// We walk through the bits of the exponent one by one. Each time we
	// see a bit, we square, thus doubling the power. If the bit is a one,
	// we also multiply by x, thus adding one to the power.

	int64_t w = 64 - int64_t(shift);
	// zz and r are used to avoid allocating in mul and div as
	// otherwise the arguments would alias.
	PackedInt64Array zz, r;
	for (int64_t j = 0; j < w; j++) {
		nat_sqr(zz, z);
		std::swap(zz, z);

		if ((v & mask) != 0) {
			nat_mul(zz, z, x);
			std::swap(zz, z);
		}

		if (!m.is_empty()) {
			nat_div(z, m, zz, r);
			std::swap(zz, q);
			std::swap(z, r);
		}

		v <<= 1;
	}

	for (int64_t i = y.size() - 2; i >= 0; i--) {
		v = y[i];

		for (int64_t j = 0; j < 64; j++) {
			nat_sqr(zz, z);
			std::swap(zz, z);

			if ((v & mask) != 0) {
				nat_mul(zz, z, x);
				std::swap(zz, z);
			}

			if (!m.is_empty()) {
				nat_div(z, m, zz, r);
				std::swap(zz, q);
				std::swap(z, r);
			}

			v <<= 1;
		}
	}

	nat_norm(z);
}

// expNNMontgomeryEven calculates x**y mod m where m = m1 × m2 for m1 = 2ⁿ and m2 odd.
// It uses two recursive calls to expNN for x**y mod m1 and x**y mod m2
// and then uses the Chinese Remainder Theorem to combine the results.
// The recursive call using m1 will use expNNWindowed,
// while the recursive call using m2 will use expNNMontgomery.
// For more details, see Ç. K. Koç, “Montgomery Reduction with Even Modulus”,
// IEE Proceedings: Computers and Digital Techniques, 141(5) 314-316, September 1994.
// http://www.people.vcu.edu/~jwang3/CMSC691/j34monex.pdf
void nat_expNNMontgomeryEven(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m) {
	// Split m = m₁ × m₂ where m₁ = 2ⁿ
	const uint64_t n = nat_trailingZeroBits(m);
	PackedInt64Array m1, m2;
	nat_lsh(m1, *natOne, n);
	nat_rsh(m2, m, n);

	// We want z = x**y mod m.
	// z₁ = x**y mod m1 = (x**y mod m) mod m1 = z mod m1
	// z₂ = x**y mod m2 = (x**y mod m) mod m2 = z mod m2
	// (We are using the math/big convention for names here,
	// where the computation is z = x**y mod m, so its parts are z1 and z2.
	// The paper is computing x = a**e mod n; it refers to these as x2 and z1.)
	PackedInt64Array z1, z2;
	nat_expNN(z1, x, y, m1, false);
	nat_expNN(z2, x, y, m2, false);

	// Reconstruct z from z₁, z₂ using CRT, using algorithm from paper,
	// which uses only a single modInverse (and an easy one at that).
	//	p = (z₁ - z₂) × m₂⁻¹ (mod m₁)
	//	z = z₂ + p × m₂
	// The final addition is in range because:
	//	z = z₂ + p × m₂
	//	  ≤ z₂ + (m₁-1) × m₂
	//	  < m₂ + (m₁-1) × m₂
	//	  = m₁ × m₂
	//	  = m.
	nat_set(z, z2);

	// Compute (z₁ - z₂) mod m1 [m1 == 2**n] into z1.
	nat_subMod2N(z1, z1, z2, n);

	// Reuse z2 for p = (z₁ - z₂) [in z1] * m2⁻¹ (mod m₁ [= 2ⁿ]).
	PackedInt64Array m2inv;
	nat_modInverse(m2inv, m2, m1);
	nat_mul(z2, z2, m2inv);
	nat_trunc(z2, z2, n);

	// Reuse z1 for p * m2.
	nat_mul(z1, z2, m2);
	nat_add(z, z, z1);

	return;
}

// expNNWindowed calculates x**y mod m using a fixed, 4-bit window,
// where m = 2**logM.
void nat_expNNWindowed(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, uint64_t logM) {
	CRASH_COND_MSG(y.size() <= 1, "big: misuse of expNNWindowed");

	if ((x[0] & 1) == 0) {
		// len(y) > 1, so y  > logM.
		// x is even, so x**y is a multiple of 2**y which is a multiple of 2**logM.
		nat_setWord(z, 0);
		return;
	}

	if (logM == 1) {
		nat_setWord(z, 1);
		return;
	}

	// zz is used to avoid allocating in mul as otherwise
	// the arguments would alias.
	PackedInt64Array zz;

	static constexpr uint64_t n = 4;
	// powers[i] contains x^i.
	PackedInt64Array powers[1 << n];
	nat_set(powers[0], *natOne);
	nat_trunc(powers[1], x, logM);
	for (int64_t i = 2; i < (1 << n); i+= 2) {
		PackedInt64Array &p2 = powers[i / 2];
		PackedInt64Array &p = powers[i];
		PackedInt64Array &p1 = powers[i + 1];
		nat_sqr(p, p2);
		nat_trunc(p, p, logM);
		nat_mul(p1, p, x);
		nat_trunc(p1, p1, logM);
	}

	// Because phi(2**logM) = 2**(logM-1), x**(2**(logM-1)) = 1,
	// so we can compute x**(y mod 2**(logM-1)) instead of x**y.
	// That is, we can throw away all but the bottom logM-1 bits of y.
	// Instead of allocating a new y, we start reading y at the right word
	// and truncate it appropriately at the start of the loop.
	int64_t i = y.size() - 1;
	const int64_t mtop = int64_t(logM - 2) / 64; // -2 because the top word of N bits is the (N-1)/W'th word.
	uint64_t mmask = ~uint64_t(0);
	const uint64_t mbits = (logM - 1) & (64 - 1);
	if (mbits != 0) {
		mmask = (1LLU << mbits) - 1;
	}
	if (i > mtop) {
		i = mtop;
	}

	bool advance = false;
	nat_setWord(z, 1);
	for (; i >= 0; i--) {
		uint64_t yi = y[i];
		if (i == mtop) {
			yi &= mmask;
		}
		for (int64_t j = 0; j < 64; j += n) {
			if (advance) {
				// Account for use of 4 bits in previous iteration.
				// Unrolled loop for significant performance
				// gain. Use go test -bench=".*" in crypto/rsa
				// to check performance before making changes.
				nat_sqr(zz, z);
				nat_trunc(z, zz, logM);

				nat_sqr(zz, z);
				nat_trunc(z, zz, logM);

				nat_sqr(zz, z);
				nat_trunc(z, zz, logM);

				nat_sqr(zz, z);
				nat_trunc(z, zz, logM);
			}

			nat_mul(zz, z, powers[yi >> (64 - n)]);
			nat_trunc(z, zz, logM);

			yi <<= n;
			advance = true;
		}
	}

	nat_norm(z);
}

// expNNMontgomery calculates x**y mod m using a fixed, 4-bit window.
// Uses Montgomery representation.
void nat_expNNMontgomery(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m) {
	const int64_t numWords = m.size();

	// We want the lengths of x and m to be equal.
	// It is OK if x >= m as long as len(x) == len(m).
	if (x.size() > numWords) {
		nat_rem(x, m, x);
		// Note: now len(x) <= numWords, not guaranteed ==.
	}
	if (x.size() < numWords) {
		x.resize(numWords);
	}

	// Ideally the precomputations would be performed outside, and reused
	// k0 = -m**-1 mod 2**_W. Algorithm from: Dumas, J.G. "On Newton–Raphson
	// Iteration for Multiplicative Inverses Modulo Prime Powers".
	uint64_t k0 = 2 - m[0];
	uint64_t t = m[0] - 1;
	for (int64_t i = 1; i < 64; i <<= 1) {
		t *= t;
		k0 *= (t + 1);
	}
	k0 = -k0;

	// RR = 2**(2*_W*len(m)) mod m
	PackedInt64Array RR, zz;
	nat_setWord(RR, 1);
	nat_lsh(zz, RR, uint64_t(2 * numWords * 64));
	nat_rem(zz, m, RR);
	if (RR.size() < numWords) {
		RR.resize(numWords);
	}

	// one = 1, with equal length to that of m
	PackedInt64Array one;
	one.resize(numWords);
	one[0] = 1;

	static constexpr int64_t n = 4;
	// powers[i] contains x^i
	PackedInt64Array powers[1 << n];
	nat_montgomery(powers[0], one, RR, m, k0, numWords);
	nat_montgomery(powers[1], x, RR, m, k0, numWords);
	for (int64_t i = 2; i < (1 << n); i++) {
		nat_montgomery(powers[i], powers[i - 1], powers[1], m, k0, numWords);
	}

	// initialize z = 1 (Montgomery 1)
	z.resize(numWords);
	nat_copy(z, 0, powers[0], 0, numWords);

	zz.resize(numWords);

	// same windowed exponent, but with Montgomery multiplications
	for (int64_t i = y.size() - 1; i >= 0; i--) {
		uint64_t yi = y[i];
		for (int64_t j = 0; j < 64; j += n) {
			if (i != y.size() - 1 || j != 0) {
				nat_montgomery(zz, z, z, m, k0, numWords);
				nat_montgomery(z, zz, zz, m, k0, numWords);
				nat_montgomery(zz, z, z, m, k0, numWords);
				nat_montgomery(z, zz, zz, m, k0, numWords);
			}
			nat_montgomery(zz, z, powers[yi >> (64 - n)], m, k0, numWords);
			std::swap(z, zz);
			yi <<= n;
		}
	}

	// convert to regular number
	nat_montgomery(zz, z, one, m, k0, numWords);

	// One last reduction, just in case.
	// See golang.org/issue/13907.
	if (nat_cmp(zz, m) >= 0) {
		// Common case is m has high bit set; in that case,
		// since zz is the same length as m, there can be just
		// one multiple of m to remove. Just subtract.
		// We think that the subtract should be sufficient in general,
		// so do that unconditionally, but double-check,
		// in case our beliefs are wrong.
		// The div is not expected to be reached.
		nat_sub(zz, zz, m);
		if (nat_cmp(zz, m) >= 0) {
			nat_rem(zz, m, zz);
		}
	}

	z = zz;
	nat_norm(z);
}

// bytes writes the value of z into buf using big-endian encoding.
// The value of z is encoded in the slice buf[i:]. If the value of z
// cannot be represented in buf, bytes panics. The number i of unused
// bytes at the beginning of buf is returned as result.
int64_t nat_bytes(PackedInt64Array z, PackedByteArray &buf) {
	// This function is used in cryptographic operations. It must not leak
	// anything but the Int's sign and bit size through side-channels. Any
	// changes must be reviewed by a security expert.
	int64_t i = buf.size();
	for (int64_t d_ : z) {
		uint64_t d = d_;
		for (size_t j = 0; j < sizeof(uint64_t); j++) {
			i--;

			if (i >= 0) {
				buf[i] = uint64_t(d);
			} else {
				CRASH_COND_MSG(uint8_t(d) != 0, "math/big: buffer too small to fit value");
			}

			d >>= 8;
		}
	}

	if (i < 0) {
		i = 0;
	}
	while (i < buf.size() && buf[i] == 0) {
		i++;
	}

	return i;
}

// bigEndianWord returns the contents of buf interpreted as a big-endian encoded Word value.
uint64_t nat_bigEndianWord(PackedByteArray buf, int64_t i) {
	return BSWAP64(buf.decode_u64(i));
}

// setBytes interprets buf as the bytes of a big-endian unsigned
// integer, sets z to that value, and returns z.
void nat_setBytes(PackedInt64Array &z, PackedByteArray buf) {
	z.resize((buf.size() + sizeof(uint64_t) - 1) / sizeof(uint64_t));

	int64_t i = buf.size();
	for (int64_t k = 0; i >= sizeof(uint64_t); k++) {
		z[k] = nat_bigEndianWord(buf, i - sizeof(uint64_t));
		i -= sizeof(uint64_t);
	}

	if (i > 0) {
		uint64_t d = 0;
		for (uint64_t s = 0; i > 0; s += 8) {
			d |= uint64_t(buf[i - 1]) << s;
			i--;
		}

		z[z.size() - 1] = d;
	}

	nat_norm(z);
}

// sqrt sets z = ⌊√x⌋
// The caller may pass stk == nil to request that sqrt obtain and release one itself.
void nat_sqrt(PackedInt64Array &z, PackedInt64Array x) {
	if (nat_cmp(x, *natOne) <= 0) {
		nat_set(z, x);
		return;
	}

	// Start with value known to be too large and repeat "z = ⌊(z + ⌊x/z⌋)/2⌋" until it stops getting smaller.
	// See Brent and Zimmermann, Modern Computer Arithmetic, Algorithm 1.13 (SqrtInt).
	// https://members.loria.fr/PZimmermann/mca/pub226.html
	// If x is one less than a perfect square, the sequence oscillates between the correct z and z+1;
	// otherwise it converges to the correct z and stays there.
	PackedInt64Array z1, z2, r;
	nat_setUint64(z1, 1);
	nat_lsh(z1, z1, uint64_t(nat_bitLen(x) + 1) / 2); // must be ≥ √x
	while (true) {
		nat_div(x, z1, z2, r);
		nat_add(z2, z2, z1);
		nat_rsh(z2, z2, 1);

		if (nat_cmp(z2, z1) >= 0) {
			// z1 is answer.
			nat_set(z, z1);
			return;
		}

		std::swap(z1, z2);
	}
}

// subMod2N returns z = (x - y) mod 2ⁿ.
void nat_subMod2N(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, uint64_t n) {
	if (nat_bitLen(x) > n) {
		nat_trunc(x, x, n);
	}
	if (nat_bitLen(y) > n) {
		nat_trunc(y, y, n);
	}

	if (nat_cmp(x, y) >= 0) {
		nat_sub(z, x, y);
		return;
	}

	// x - y < 0; x - y mod 2ⁿ = x - y + 2ⁿ = 2ⁿ - (y - x) = 1 + 2ⁿ-1 - (y - x) = 1 + ^(y - x).
	nat_sub(z, y, x);
	while (uint64_t(z.size()) * 64 < n) {
		z.append(0);
	}

	for (int64_t i = 0; i < z.size(); i++) {
		z[i] = ~z[i];
	}

	nat_trunc(z, z, n);
	nat_add(z, z, *natOne);
}
