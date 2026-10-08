// This file is ported from src/math/big/nat.go in Go 1.27.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements unsigned multi-precision integers (natural
// numbers). They are the building blocks for the implementation
// of signed integers, rationals, and floating-point numbers.
//
// Caution: This implementation relies on the function "alias"
//          which assumes that (nat) slice capacities are never
//          changed (no 3-operand slice expressions). If that
//          changes, alias needs to be updated for correctness.

#include "godot_big_naturals.h"

using namespace godot;

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

String BigNat::String() const {
	return "0x" + itoa(false, 16).get_string_from_ascii();
}

void BigNat::norm() {
	int64_t i = array.size();

	while (i > 0 && operator[](i - 1) == 0) {
		i--;
	}

	array.resize(i);
}

void BigNat::setUint64(uint64_t p_x) {
	if (p_x == 0) {
		array.clear();
		return;
	}

	array.resize(1);
	operator[](0) = p_x;
}

void BigNat::add(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	int64_t m = p_x.array.size();
	int64_t n = p_y.array.size();

	if (m < n) {
		SWAP(p_x, p_y);
		SWAP(m, n);
	}
	if (m == 0) {
		// n == 0 because m >= n; result is 0
		array.clear();
		return;
	}
	if (n == 0) {
		// result is x
		*this = std::move(p_x);
		return;
	}
	// m > 0

	array.resize(m + 1);
	BigWord c = addVV(slice(0, n), p_x.cslice(0, n), p_y.cslice(0, n));
	if (m > n) {
		c = addVW(slice(n, m), p_x.cslice(n), c);
	}
	operator[](m) = c;

	norm();
}

void BigNat::add1(BigNat p_x) {
	*this = std::move(p_x);

	int64_t *p = array.ptrw();

	int64_t i = 0;
	while (i < array.size()) {
		if (likely(p[i] != -1)) {
			p[i]++;
			return;
		}

		p[i] = 0;
	}

	array.append(1);
}

void BigNat::sub(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	const int64_t m = p_x.array.size();
	const int64_t n = p_y.array.size();

	CRASH_COND_MSG(m < n, "underflow");
	if (m == 0) {
		// n == 0 because m >= n; result is 0
		array.clear();
		return;
	}
	if (n == 0) {
		// result is x
		*this = p_x;
		return;
	}
	// m > 0

	array.resize(m);
	BigWord c = subVV(slice(0, n), p_x.cslice(0, n), p_y.cslice(0, n));
	if (m > n) {
		c = subVW(slice(n), p_x.cslice(n), c);
	}
	CRASH_COND_MSG(c != 0, "underflow");

	norm();
}

void BigNat::sub1(BigNat p_x) {
	*this = std::move(p_x);

	CRASH_COND_MSG(is_zero(), "underflow");

	int64_t *p = array.ptrw();

	int64_t i = 0;
	while (i < array.size()) {
		if (likely(p[i] != 0)) {
			p[i]--;
			norm();
			return;
		}

		p[i] = -1;
	}

	CRASH_NOW_MSG("unreachable");
}

int BigNat::cmp(const BigNat &p_y) const {
	const int64_t m = array.size();
	const int64_t n = p_y.array.size();
	if (m != n || m == 0) {
		if (m < n) {
			return -1;
		}
		if (m > n) {
			return +1;
		}
		return 0;
	}

	int64_t i = m - 1;
	while (i > 0 && operator[](i) == p_y[i]) {
		i--;
	}

	const BigWord x = operator[](i);
	const BigWord y = p_y[i];
	if (x < y) {
		return -1;
	}
	if (x > y) {
		return +1;
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
void BigNat::montgomery(BigNat p_x, BigNat p_y, BigNat p_m, BigWord p_k, int64_t p_n) {
	// This code assumes x, y, m are all the same length, n.
	// (required by addMulVVW and the for loop).
	// It also assumes that x, y are already reduced mod m,
	// or else the result will not be properly reduced.
	CRASH_COND_MSG(p_x.array.size() != p_n || p_y.array.size() != p_n || p_m.array.size() != p_n, "math/big: mismatched montgomery number lengths");
	array.resize(p_n * 2);
	memset(ptrw(), 0, array.size() * sizeof(BigWord));

	BigWord c = 0;
	for (int64_t i = 0; i < p_n; i++) {
		const BigWord d = p_y[i];
		const BigWord c2 = addMulVVWW(slice(i, p_n + i), cslice(i, p_n + i), p_x.cslice(), d, 0);
		const BigWord t = operator[](i) * p_k;
		const BigWord c3 = addMulVVWW(slice(i, p_n + i), cslice(i, p_n + i), p_m.cslice(), t, 0);
		const BigWord cx = c + c2;
		const BigWord cy = cx + c3;
		operator[](p_n + i) = cy;
		c = (cx < c2 || cy < c3) ? 1 : 0;
	}

	if (c != 0) {
		subVV(slice(0, p_n), cslice(p_n), p_m.cslice());
		array.resize(p_n);
	} else {
		array = array.slice(p_n);
	}
}

// addTo implements z += x; z must be long enough.
// (we don't use nat.add because we need z to stay the same
// slice, and we don't need to normalize z after each addition)
void BigNat::addTo(Slice r_z, const CSlice &p_x) {
	const int64_t n = p_x.size();
	if (n <= 0) {
		return;
	}

	const BigWord c = addVV(r_z.slice(0, n), r_z.cslice(0, n), p_x);
	if (c != 0) {
		addVW(r_z.slice(n), r_z.cslice(n), c);
	}
}

// mulRange computes the product of all the unsigned integers in the
// range [a, b] inclusively. If a > b (empty range), the result is 1.
// The caller may pass stk == nil to request that mulRange obtain and release one itself.
void BigNat::mulRange(uint64_t p_a, uint64_t p_b) {
	if (p_a == 0) {
		// cut long ranges short (optimization)
		setUint64(0);
		return;
	}
	if (p_a > p_b) {
		setUint64(1);
		return;
	}
	if (p_a == p_b) {
		setUint64(p_a);
		return;
	}
	if (p_a + 1 == p_b) {
		mul({ p_a }, { p_b });
		return;
	}

	const uint64_t m = p_a + ((p_b - p_a) / 2); // avoid overflow

	BigNat lo;
	BigNat hi;
	lo.mulRange(p_a, m);
	hi.mulRange(m + 1, p_b);

	mul(lo, hi);
}

// bitLen returns the length of x in bits.
// Unlike most methods, it works even if x is not normalized.
int64_t BigNat::bitLen() const {
	// This function is used in cryptographic operations. It must not leak
	// anything but the Int's sign and bit size through side-channels. Any
	// changes must be reviewed by a security expert.
	const int64_t i = array.size() - 1;
	if (i >= 0) {
		// bits.Len uses a lookup table for the low-order bits on some
		// architectures. Neutralize any input-dependent behavior by setting all
		// bits after the first one bit.
		uint64_t top = static_cast<uint64_t>(operator[](i));
		top |= top >> 1;
		top |= top >> 2;
		top |= top >> 4;
		top |= top >> 8;
		top |= top >> 16;
		top |= top >> 16 >> 16; // ">> 32" doesn't compile on 32-bit architectures
		return (i * 64) + (64 - std::countl_zero(top));
	}

	return 0;
}

// trailingZeroBits returns the number of consecutive least significant zero
// bits of x.
uint64_t BigNat::trailingZeroBits() const {
	if (is_zero()) {
		return 0;
	}

	int64_t i = 0;
	while (operator[](i) == 0) {
		i++;
	}
	// x[i] != 0
	return (i * 64) + std::countr_zero(uint(operator[](i)));
}

// isPow2 returns i, true when x == 2**i and 0, false otherwise.
bool BigNat::isPow2(uint64_t &r_p) const {
	uint64_t i = 0;
	while (operator[](i) == 0) {
		i++;
	}

	if (i == static_cast<uint64_t>(array.size()) - 1 && (operator[](i) & (operator[](i) - 1)) == 0) {
		r_p = (i * 64) + static_cast<uint64_t>(std::countr_zero(operator[](i)));
		return true;
	}

	r_p = 0;
	return false;
}

// z = x << s
void BigNat::lsh(BigNat p_x, uint64_t p_s) {
	if (p_s == 0) {
		*this = std::move(p_x);
		return;
	}

	const int64_t m = p_x.array.size();
	if (m == 0) {
		array.clear();
		return;
	}
	// m > 0

	const int64_t n = m + static_cast<int64_t>(p_s / 64);
	array.resize(n + 1);
	p_s %= 64;
	if (p_s == 0) {
		memcpy(ptrw() + (n - m), p_x.ptr(), m * sizeof(BigWord));
		operator[](n) = 0;
	} else {
		operator[](n) = lshVU(slice(n - m, n), p_x.cslice(), p_s);
	}
	memset(ptrw(), 0, (n - m) * sizeof(BigWord));

	norm();
}

// z = x >> s
void BigNat::rsh(BigNat p_x, uint64_t p_s) {
	if (p_s == 0) {
		*this = std::move(p_x);
		return;
	}

	const int64_t m = p_x.array.size();
	const int64_t n = m - static_cast<int64_t>(p_s / 64);
	if (n <= 0) {
		array.clear();
		return;
	}
	// n > 0

	array.resize(n);
	p_s %= 64;
	if (p_s == 0) {
		memcpy(ptrw(), p_x.ptr() + (m - n), n * sizeof(BigWord));
	} else {
		rshVU(slice(), p_x.cslice(m - n), p_s);
	}

	norm();
}

void BigNat::setBit(BigNat p_x, uint64_t p_i, uint64_t p_b) { // NOLINT(performance-unnecessary-value-param)
	const int64_t j = static_cast<int64_t>(p_i / 64);
	const BigWord m = UINT64_C(1) << (p_i % 64);
	const int64_t n = p_x.array.size();
	switch (p_b) {
		case 0:
			array.resize(n);
			memcpy(ptrw(), p_x.ptr(), n * sizeof(BigWord));
			if (j >= n) {
				// no need to grow
				return;
			}
			operator[](j) &= ~m;
			norm();
			return;
		case 1:
			if (j >= n) {
				array.resize(j + 1);
				memset(ptrw() + n, 0, (j + 1 - n) * sizeof(BigWord));
			} else {
				array.resize(n);
			}
			memcpy(ptrw(), p_x.ptr(), n * sizeof(BigWord));
			operator[](j) |= m;
			// no need to normalize
			return;
		default:
			CRASH_NOW_MSG("set bit is not 0 or 1");
	}
}

// bit returns the value of the i'th bit, with lsb == bit 0.
uint64_t BigNat::bit(uint64_t p_i) const {
	const uint64_t j = p_i / 64;
	if (j >= static_cast<uint64_t>(array.size())) {
		return 0;
	}
	// 0 <= j < len(x)

	return static_cast<uint64_t>((operator[](j) >> (p_i % 64)) & 1);
}

// sticky returns 1 if there's a 1 bit within the
// i least significant bits, otherwise it returns 0.
uint64_t BigNat::sticky(uint64_t p_i) const {
	uint64_t j = p_i / 64;

	if (j >= static_cast<uint64_t>(array.size())) {
		return is_zero() ? 0 : 1;
	}
	// 0 <= j < len(x)

	for (uint64_t k = 0; k < j; k++) {
		if (operator[](k) != 0) {
			return 1;
		}
	}

	p_i %= 64;
	BigWord mask = (UINT64_C(1) << p_i) - 1;
	if (p_i == 0) {
		mask = UINT64_MAX;
	}

	return (operator[](j) & mask) != 0 ? 1 : 0;
}

void BigNat::and_(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	const int64_t n = p_y.array.size();
	const int64_t m = Math::min(p_x.array.size(), n);
	// m <= n

	array.resize(m);
	BigWord *z = ptrw();
	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();
	for (int64_t i = 0; i < m; i++) {
		z[i] = x[i] & y[i];
	}

	norm();
}

// trunc returns z = x mod 2ⁿ.
void BigNat::trunc(BigNat p_x, uint64_t p_n) {
	const uint64_t w = (p_n + 64 - 1) / 64;
	*this = std::move(p_x);

	if (static_cast<uint64_t>(array.size()) < w) {
		return;
	}

	array.resize(w);
	if (p_n % 64 != 0) {
		operator[](w - 1) &= (UINT64_C(1) << (p_n % 64)) - 1;
	}

	norm();
}

void BigNat::andNot(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	const int64_t m = p_x.array.size();
	const int64_t n = Math::min(m, p_y.array.size());
	// m >= n

	array.resize(m);
	BigWord *z = ptrw();
	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();
	for (int64_t i = 0; i < n; i++) {
		z[i] = x[i] & ~y[i];
	}
	memcpy(z + n, x + n, (m - n) * sizeof(BigWord));

	norm();
}

void BigNat::or_(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	int64_t m = p_x.array.size();
	int64_t n = p_y.array.size();
	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();
	const BigWord *s = x;
	if (m < n) {
		SWAP(m, n);
		s = y;
	}
	// m >= n

	array.resize(m);
	BigWord *z = ptrw();

	for (int64_t i = 0; i < n; i++) {
		z[i] = x[i] | y[i];
	}
	memcpy(z + n, s + n, m - n);

	norm();
}

void BigNat::xor_(BigNat p_x, BigNat p_y) { // NOLINT(performance-unnecessary-value-param)
	int64_t m = p_x.array.size();
	int64_t n = p_y.array.size();
	const BigWord *s = p_x.ptr();
	if (m < n) {
		SWAP(m, n);
		s = p_y.ptr();
	}
	// m >= n

	array.resize(m);

	BigWord *z = ptrw();
	const BigWord *x = p_x.ptr();
	const BigWord *y = p_y.ptr();
	for (int64_t i = 0; i < n; i++) {
		z[i] = x[i] ^ y[i];
	}
	memcpy(z + n, s + n, (m - n) * sizeof(BigWord));

	norm();
}

// random creates a random integer in [0..limit), using the space in z if
// possible. n is the bit length of limit.
void BigNat::random(const std::function<uint32_t()> &p_rand, BigNat p_limit, int64_t p_n) { // NOLINT(performance-unnecessary-value-param)
	array.resize(p_limit.array.size());
	const int64_t last = p_limit.array.size() - 1;

	[[maybe_unused]] uint64_t bitLengthOfMSW = p_n % 64;
	BigWord mask = (UINT64_C(1) << bitLengthOfMSW) - 1;
	if (bitLengthOfMSW == 0) {
		bitLengthOfMSW = 64;
		mask = UINT64_MAX;
	}

	BigWord *z = ptrw();
	for (;;) {
		for (int64_t i = 0; i <= last; i++) {
			z[i] = static_cast<BigWord>(p_rand()) | (static_cast<BigWord>(p_rand()) << 32);
		}
		z[last] &= mask;

		if (cmp(p_limit) < 0) {
			break;
		}
	}

	norm();
}

// If m != 0 (i.e., len(m) != 0), expNN sets z to x**y mod m;
// otherwise it sets z to x**y. The result is the value of z.
// The caller may pass stk == nil to request that expNN obtain and release one itself.
void BigNat::expNN(BigNat p_x, BigNat p_y, BigNat p_m, bool p_slow) { // NOLINT(performance-unnecessary-value-param)
	// x**y mod 1 == 0
	if (p_m.array.size() == 1 && p_m[0] == 1) {
		array.clear();
		return;
	}
	// m == 0 || m > 1

	// x**0 == 1
	if (p_y.is_zero()) {
		setUint64(1);
		return;
	}
	// y > 0

	// 0**y = 0
	if (p_x.is_zero()) {
		array.clear();
		return;
	}
	// x > 0

	// 1**y = 1
	if (p_x.array.size() == 1 && p_x[0] == 1) {
		setUint64(1);
		return;
	}
	// x > 1

	// x**1 == x
	if (p_y.array.size() == 1 && p_y[0] == 1 && p_m.is_zero()) {
		*this = std::move(p_x);
		return;
	}

	if (p_y.array.size() == 1 && p_y[0] == 1) { // len(m) > 0
		rem(p_x, p_m);
		return;
	}

	// y > 1

	if (!p_m.is_zero()) {
		// We likely end up being as long as the modulus.
		array.resize(p_m.array.size());

		// If the exponent is large, we use the Montgomery method for odd values,
		// and a 4-bit, windowed exponentiation for powers of two,
		// and a CRT-decomposed Montgomery method for the remaining values
		// (even values times non-trivial odd values, which decompose into one
		// instance of each of the first two cases).
		if (p_y.array.size() > 1 && !p_slow) {
			if ((p_m[0] & 1) == 1) {
				expNNMontgomery(p_x, p_y, p_m);
				return;
			}

			uint64_t logM;
			if (p_m.isPow2(logM)) {
				expNNWindowed(p_x, p_y, logM);
				return;
			}

			expNNMontgomeryEven(p_x, p_y, p_m);
			return;
		}
	}

	BigNat z{ p_x };
	BigWord v = p_y[p_y.array.size() - 1]; // v > 0 because y is normalized and y > 0
	const int64_t shift = std::countl_zero(v) + 1;
	v <<= shift;
	BigNat q;

	static constexpr uint64_t mask = UINT64_C(1) << (64 - 1);

	// We walk through the bits of the exponent one by one. Each time we
	// see a bit, we square, thus doubling the power. If the bit is a one,
	// we also multiply by x, thus adding one to the power.

	int64_t w = 64 - shift;
	// zz and r are used to avoid allocating in mul and div as
	// otherwise the arguments would alias.
	BigNat zz;
	BigNat r;
	for (int64_t j = 0; j < w; j++) {
		zz.sqr(z);
		SWAP(z, zz);

		if ((v & mask) != 0) {
			zz.mul(z, p_x);
			SWAP(z, zz);
		}

		if (!p_m.is_zero()) {
			zz.div(z, p_m, r);
			SWAP(zz, q);
			SWAP(z, r);
		}

		v <<= 1;
	}

	for (int64_t i = p_y.array.size() - 2; i >= 0; i--) {
		v = p_y[i];

		for (int64_t j = 0; j < 64; j++) {
			zz.sqr(z);
			SWAP(z, zz);

			if ((v & mask) != 0) {
				zz.mul(z, p_x);
				SWAP(z, zz);
			}

			if (!p_m.is_zero()) {
				zz.div(z, p_m, r);
				SWAP(zz, q);
				SWAP(z, r);
			}

			v <<= 1;
		}
	}

	*this = std::move(z);

	norm();
}

// expNNMontgomeryEven calculates x**y mod m where m = m1 × m2 for m1 = 2ⁿ and m2 odd.
// It uses two recursive calls to expNN for x**y mod m1 and x**y mod m2
// and then uses the Chinese Remainder Theorem to combine the results.
// The recursive call using m1 will use expNNWindowed,
// while the recursive call using m2 will use expNNMontgomery.
// For more details, see Ç. K. Koç, “Montgomery Reduction with Even Modulus”,
// IEE Proceedings: Computers and Digital Techniques, 141(5) 314-316, September 1994.
// http://www.people.vcu.edu/~jwang3/CMSC691/j34monex.pdf
void BigNat::expNNMontgomeryEven(const BigNat &p_x, const BigNat &p_y, const BigNat &p_m) {
	// Split m = m₁ × m₂ where m₁ = 2ⁿ
	const uint64_t n = p_m.trailingZeroBits();
	BigNat m1;
	BigNat m2;
	m1.lsh({ 1 }, n);
	m2.rsh(p_m, n);

	// We want z = x**y mod m.
	// z₁ = x**y mod m1 = (x**y mod m) mod m1 = z mod m1
	// z₂ = x**y mod m2 = (x**y mod m) mod m2 = z mod m2
	// (We are using the math/big convention for names here,
	// where the computation is z = x**y mod m, so its parts are z1 and z2.
	// The paper is computing x = a**e mod n; it refers to these as x2 and z1.)
	BigNat z1;
	BigNat z2;
	z1.expNN(p_x, p_y, m1, false);
	z2.expNN(p_x, p_y, m2, false);

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
	*this = z2;

	// Compute (z₁ - z₂) mod m1 [m1 == 2**n] into z1.
	z1.subMod2N(z1, z2, n);

	// Reuse z2 for p = (z₁ - z₂) [in z1] * m2⁻¹ (mod m₁ [= 2ⁿ]).
	BigNat m2inv;
	m2inv.modInverse(m2, m1);
	z2.mul(z1, m2inv);
	z2.trunc(z2, n);

	// Reuse z1 for p * m2.
	z1.mul(z2, m2);
	add(*this, z1);
}

// expNNWindowed calculates x**y mod m using a fixed, 4-bit window,
// where m = 2**logM.
void BigNat::expNNWindowed(const BigNat &p_x, const BigNat &p_y, uint64_t p_logM) {
	CRASH_COND_MSG(p_y.array.size() <= 1, "big: misuse of expNNWindowed");
	if ((p_x[0] & 1) == 0) {
		// len(y) > 1, so y  > logM.
		// x is even, so x**y is a multiple of 2**y which is a multiple of 2**logM.
		array.clear();
		return;
	}
	if (p_logM == 1) {
		setUint64(1);
		return;
	}

	// zz is used to avoid allocating in mul as otherwise
	// the arguments would alias.
	BigNat zz;

	static constexpr uint64_t n = 4;
	// powers[i] contains x^i.
	std::array<BigNat, (1 << n)> powers;
	powers.at(0) = { 1 };
	powers.at(1).trunc(p_x, p_logM);
	for (int i = 2; i < (1 << n); i += 2) {
		BigNat &p2 = powers.at(i / 2);
		BigNat &p = powers.at(i);
		BigNat &p1 = powers.at(i + 1);
		p.sqr(p2);
		p.trunc(p, p_logM);
		p1.mul(p, p_x);
		p1.trunc(p1, p_logM);
	}

	// Because phi(2**logM) = 2**(logM-1), x**(2**(logM-1)) = 1,
	// so we can compute x**(y mod 2**(logM-1)) instead of x**y.
	// That is, we can throw away all but the bottom logM-1 bits of y.
	// Instead of allocating a new y, we start reading y at the right word
	// and truncate it appropriately at the start of the loop.
	int64_t i = p_y.array.size() - 1;
	int64_t mtop = static_cast<int64_t>((p_logM - 2) / 64); // -2 because the top word of N bits is the (N-1)/W'th word.
	BigWord mmask = UINT64_MAX;
	uint64_t mbits = (p_logM - 1) & (64 - 1);
	if (mbits != 0) {
		mmask = (UINT64_C(1) << mbits) - 1;
	}
	i = Math::min(i, mtop);
	bool advance = false;
	setUint64(1);
	for (; i >= 0; i--) {
		BigWord yi = p_y[i];
		if (i == mtop) {
			yi &= mmask;
		}
		for (int64_t j = 0; j < 64; j += n) {
			if (advance) {
				// Account for use of 4 bits in previous iteration.
				// Unrolled loop for significant performance
				// gain. Use go test -bench=".*" in crypto/rsa
				// to check performance before making changes.
				static_assert(n == 4);

				zz.sqr(*this);
				trunc(zz, p_logM);

				zz.sqr(*this);
				trunc(zz, p_logM);

				zz.sqr(*this);
				trunc(zz, p_logM);

				zz.sqr(*this);
				trunc(zz, p_logM);
			}

			zz.mul(*this, powers.at(yi >> (64 - n)));
			trunc(zz, p_logM);

			yi <<= n;
			advance = true;
		}
	}

	norm();
}

// expNNMontgomery calculates x**y mod m using a fixed, 4-bit window.
// Uses Montgomery representation.
void BigNat::expNNMontgomery(const BigNat &p_x, const BigNat &p_y, const BigNat &p_m) {
	int64_t numWords = p_m.array.size();

	// We want the lengths of x and m to be equal.
	// It is OK if x >= m as long as len(x) == len(m).
	BigNat x = p_x;
	if (x.array.size() > numWords) {
		x.rem(x, p_m);
		// Note: now len(x) <= numWords, not guaranteed ==.
	}

	if (x.array.size() < numWords) {
		x.array.resize(numWords);
	}

	// Ideally the precomputations would be performed outside, and reused
	// k0 = -m**-1 mod 2**_W. Algorithm from: Dumas, J.G. "On Newton–Raphson
	// Iteration for Multiplicative Inverses Modulo Prime Powers".
	BigWord k0 = 2 - p_m[0];
	BigWord t = p_m[0] - 1;
	for (int64_t i = 1; i < 64; i <<= 1) {
		t *= t;
		k0 *= (t + 1);
	}
	k0 = -k0;

	// RR = 2**(2*_W*len(m)) mod m
	BigNat RR{ 1 };
	BigNat zz;
	zz.lsh(RR, 2 * numWords * 64);
	RR.rem(zz, p_m);
	if (RR.array.size() < numWords) {
		RR.array.resize(numWords);
	}
	// one = 1, with equal length to that of m
	BigNat one{ 1 };
	one.array.resize(numWords);

	static constexpr int64_t n = 4;
	// powers[i] contains x^i
	std::array<BigNat, (1 << n)> powers;
	powers.at(0).montgomery(one, RR, p_m, k0, numWords);
	powers.at(1).montgomery(x, RR, p_m, k0, numWords);
	for (int64_t i = 2; i < (1 << n); i++) {
		powers.at(i).montgomery(powers.at(i - 1), powers.at(1), p_m, k0, numWords);
	}

	// initialize z = 1 (Montgomery 1)
	*this = powers.at(0);
	zz.array.resize(numWords);

	// same windowed exponent, but with Montgomery multiplications
	for (int64_t i = p_y.array.size() - 1; i >= 0; i--) {
		BigWord yi = p_y[i];
		for (int64_t j = 0; j < 64; j += n) {
			if (i != p_y.array.size() - 1 || j != 0) {
				zz.montgomery(*this, *this, p_m, k0, numWords);
				montgomery(zz, zz, p_m, k0, numWords);
				zz.montgomery(*this, *this, p_m, k0, numWords);
				montgomery(zz, zz, p_m, k0, numWords);
			}
			zz.montgomery(*this, powers.at(yi >> (64 - n)), p_m, k0, numWords);
			SWAP(*this, zz);
			yi <<= n;
		}
	}
	// convert to regular number
	zz.montgomery(*this, one, p_m, k0, numWords);

	// One last reduction, just in case.
	// See golang.org/issue/13907.
	if (zz.cmp(p_m) >= 0) {
		// Common case is m has high bit set; in that case,
		// since zz is the same length as m, there can be just
		// one multiple of m to remove. Just subtract.
		// We think that the subtract should be sufficient in general,
		// so do that unconditionally, but double-check,
		// in case our beliefs are wrong.
		// The div is not expected to be reached.
		zz.sub(zz, p_m);
		if (unlikely(zz.cmp(p_m) >= 0)) {
			zz.rem(zz, p_m);
		}
	}

	*this = std::move(zz);

	norm();
}

// bytes writes the value of z into buf using big-endian encoding.
// The value of z is encoded in the slice buf[i:]. If the value of z
// cannot be represented in buf, bytes panics. The number i of unused
// bytes at the beginning of buf is returned as result.
int64_t BigNat::bytes(PackedByteArray &r_buf) const {
	// This function is used in cryptographic operations. It must not leak
	// anything but the Int's sign and bit size through side-channels. Any
	// changes must be reviewed by a security expert.
	int64_t i = r_buf.size();
	for (int64_t j = 0; j < array.size(); j++) {
		BigWord d = operator[](j);
		for (size_t k = 0; k < sizeof(BigWord); k++) {
			i--;
			if (i >= 0) {
				r_buf[i] = static_cast<uint8_t>(d);
			} else {
				CRASH_COND_MSG(static_cast<uint8_t>(d) != 0, "buffer too small to fit value");
			}
			d >>= 8;
		}
	}

	i = Math::max(i, INT64_C(0));
	while (i < r_buf.size() && r_buf[i] == 0) {
		i++;
	}

	return i;
}

// setBytes interprets buf as the bytes of a big-endian unsigned
// integer, sets z to that value, and returns z.
void BigNat::setBytes(const PackedByteArray &p_buf) {
	int64_t i = p_buf.size();
	array.resize((i + sizeof(BigWord) - 1) / sizeof(BigWord));

	for (int64_t k = 0; i >= sizeof(BigWord); k++) {
		i -= sizeof(BigWord);
		operator[](k) = BSWAP64(p_buf.decode_u64(i));
	}

	if (i > 0) {
		BigWord d = 0;
		for (uint64_t s = 0; i > 0; s += 8) {
			d |= static_cast<BigWord>(p_buf[i - 1]) << s;
			i--;
		}
		operator[](array.size() - 1) = d;
	}

	norm();
}

// sqrt sets z = ⌊√x⌋
// The caller may pass stk == nil to request that sqrt obtain and release one itself.
void BigNat::sqrt(BigNat p_x) {
	if (p_x.cmp({ 1 }) <= 0) {
		*this = std::move(p_x);
		return;
	}

	// Start with value known to be too large and repeat "z = ⌊(z + ⌊x/z⌋)/2⌋" until it stops getting smaller.
	// See Brent and Zimmermann, Modern Computer Arithmetic, Algorithm 1.13 (SqrtInt).
	// https://members.loria.fr/PZimmermann/mca/pub226.html
	// If x is one less than a perfect square, the sequence oscillates between the correct z and z+1;
	// otherwise it converges to the correct z and stays there.
	BigNat z1{ 1 };
	BigNat z2;
	BigNat r;
	z1.lsh(z1, static_cast<uint64_t>(p_x.bitLen() + 1) / 2); // must be ≥ √x
	for (int64_t n = 0;; n++) {
		z2.div(p_x, z1, r);
		z2.add(z2, z1);
		z2.rsh(z2, 1);
		if (z2.cmp(z1) >= 0) {
			// z1 is answer.
			*this = std::move(z1);
			return;
		}
		SWAP(z1, z2);
	}
}

// subMod2N returns z = (x - y) mod 2ⁿ.
void BigNat::subMod2N(BigNat p_x, BigNat p_y, uint64_t p_n) {
	if (static_cast<uint64_t>(p_x.bitLen()) > p_n) {
		// ok to overwrite x in place
		p_x.trunc(p_x, p_n);
	}
	if (static_cast<uint64_t>(p_y.bitLen()) > p_n) {
		// ok to overwrite y in place
		p_y.trunc(p_y, p_n);
	}
	if (p_x.cmp(p_y) >= 0) {
		sub(p_x, p_y);
		return;
	}
	// x - y < 0; x - y mod 2ⁿ = x - y + 2ⁿ = 2ⁿ - (y - x) = 1 + 2ⁿ-1 - (y - x) = 1 + ^(y - x).
	sub(p_y, p_x);
	while (static_cast<uint64_t>(array.size()) * 64 < p_n) {
		array.append(0);
	}
	for (int64_t i = 0; i < array.size(); i++) {
		operator[](i) = ~operator[](i);
	}
	trunc(*this, p_n);
	add1(*this);
}
