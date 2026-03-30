// This file is ported from src/math/big/nat.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"

using namespace godot;

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

void BigNat::norm() {
	int64_t i = array.size();
	while (i > 0 && array[i - 1] == 0) {
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
	array[0] = p_x;
}

void BigNat::set(BigNat p_x) {
	array = p_x.array;
}

void BigNat::add(BigNat p_x, BigNat p_y) {
	const int64_t m = p_x.array.size();
	const int64_t n = p_y.array.size();

	if (m < n) {
		add(p_y, p_x);
		return;
	}
	if (m == 0) {
		// n == 0 because m >= n; result is 0
		array.clear();
		return;
	}
	if (n == 0) {
		// result is x
		set(p_x);
		return;
	}
	// m > 0

	array.resize(n);
	BigWord c = addVV(*this, BigNat{p_x.array.slice(0, n)}, BigNat{p_y.array.slice(0, n)});
	if (m > n) {
		BigNat z;
		z.array.resize(m - n);
		c = addVW(z, BigNat{p_x.array.slice(n)}, c);
		array.append_array(z.array);
	}
	array.append(c);

	norm();
}

void BigNat::sub(BigNat p_x, BigNat p_y) {
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
		set(p_x);
		return;
	}
	// m > 0

	array.resize(n);
	BigWord c = subVV(*this, BigNat{p_x.array.slice(0, n)}, BigNat{p_y.array.slice(0, n)});
	if (m > n) {
		BigNat z;
		z.array.resize(m - n);
		c = subVW(z, BigNat{p_x.array.slice(n)}, c);
	}
	CRASH_COND_MSG(c != 0, "underflow");

	norm();
}

int BigNat::cmp(BigNat p_y) const {
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
	while (i > 0 && (*this)[i] == p_y[i]) {
		i--;
	}

	if ((*this)[i] < p_y[i]) {
		return -1;
	}
	if ((*this)[i] > p_y[i]) {
		return +1;
	}
	return 0;
}

int BigNat::cmpnorm(BigNat p_y) const {
	p_y.norm();
	return cmp(p_y);
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
	CRASH_COND(p_x.array.size() != p_n || p_y.array.size() != p_n || p_m.array.size() != p_n);

	array.resize(p_n * 2);
	array.fill(0);

	BigWord c = 0;
	for (int64_t i = 0; i < p_n; i++) {
		const BigWord d = p_y[i];
		BigNat z{array.slice(i, p_n + i)};
		const BigWord c2 = addMulVVWW(z, z, p_x, d, 0);
		const BigWord t = z[0] * p_k;
		const BigWord c3 = addMulVVWW(z, z, p_m, t, 0);
		const BigWord cx = c + c2;
		const BigWord cy = cx + c3;
		for (int64_t j = 0; j < p_n; j++) {
			(*this)[i + j] = z[j];
		}
		(*this)[p_n + i] = cy;
		if (cx < c2 || cy < c3) {
			c = 1;
		} else {
			c = 0;
		}
	}

	if (c != 0) {
		BigNat z{array.slice(p_n)};
		array.resize(p_n);
		subVV(*this, z, p_m);
	} else {
		array = array.slice(p_n);
	}
}

// addTo implements z += x; z must be long enough.
// (we don't use nat.add because we need z to stay the same
// slice, and we don't need to normalize z after each addition)
void BigNat::addTo(BigNat &r_z, int64_t p_start, BigNat p_x) {
	const int64_t n = p_x.array.size();
	if (n > 0) {
		BigNat z0{r_z.array.slice(p_start, p_start + n)};
		BigNat z1{r_z.array.slice(p_start + n)};
		r_z.array.resize(p_start);
		BigWord c = addVV(z0, z0, p_x);
		if (c != 0 && !z1.array.is_empty()) {
			addVW(z1, z1, c);
		}
		r_z.array.append_array(z0.array);
		r_z.array.append_array(z1.array);
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
	BigNat a, b;
	if (p_a + 1 == p_b) {
		a.setUint64(p_a);
		b.setUint64(p_b);
		mul(a, b);
		return;
	}

	const uint64_t m = p_a + (p_b - p_a) / 2; // avoid overflow
	a.mulRange(p_a, m);
	b.mulRange(m + 1, p_b);
	mul(a, b);
}

// bitLen returns the length of x in bits.
// Unlike most methods, it works even if x is not normalized.
int64_t BigNat::bitLen() const {
	int64_t i = array.size() - 1;
	if (i >= 0) {
		if (array[i] == 0) {
			return i * 64;
		}

		return i * 64 + 64 - std::countl_zero(uint64_t(array[i]));
	}

	return 0;
}

// trailingZeroBits returns the number of consecutive least significant zero
// bits of x.
uint64_t BigNat::trailingZeroBits() const {
	if (array.is_empty()) {
		return 0;
	}
	uint64_t i = 0;
	while (array[i] == 0) {
		i++;
	}
	// x[i] != 0
	return i * 64 + std::countr_zero(uint64_t(array[i]));
}

// isPow2 returns i, true when x == 2**i and 0, false otherwise.
Pair<uint64_t, bool> BigNat::isPow2() const {
	int64_t i = 0;
	while ((*this)[i] == 0) {
		i++;
	}
	if (i == array.size() && ((*this)[i] & ((*this)[i] - 1)) == 0) {
		return {uint64_t(i * 64) + std::countr_zero((*this)[i]), true};
	}
	return {0, false};
}

// z = x << s
void BigNat::lsh(BigNat p_x, uint64_t p_s) {
	if (p_s == 0) {
		set(p_x);
		return;
	}

	int64_t m = p_x.array.size();
	if (m == 0) {
		array.clear();
		return;
	}
	// m > 0

	int64_t n = m + int64_t(p_s / 64);
	p_s %= 64;
	if (p_s == 0) {
		array.resize(n + 1);
		memcpy(array.ptrw() + n - m, p_x.array.ptr(), m * 8);
		array[n] = 0;
	} else {
		array.resize(n - m);
		BigNat z;
		z.array.resize(n - m);
		z.array.append(lshVU(z, p_x, p_s));
		array.append_array(z.array);
	}

	for (int64_t i = 0; i < n - m; i++) {
		array[i] = 0;
	}

	norm();
}

// z = x >> s
void BigNat::rsh(BigNat p_x, uint64_t p_s) {
	if (p_s == 0) {
		array = p_x.array;
		return;
	}

	int64_t m = p_x.array.size();
	int64_t n = m - int64_t(p_s / 64);
	if (n <= 0) {
		array.clear();
		return;
	}
	// n > 0

	p_s %= 64;
	if (p_s == 0) {
		array = p_x.array.slice(m - n);
	} else {
		array.resize(n);
		rshVU(*this, BigNat{p_x.array.slice(m - n)}, p_s);
	}

	norm();
}

void BigNat::setBit(BigNat p_x, uint64_t p_i, uint64_t p_b) {
	int64_t j = p_i / 64;
	BigWord m = BigWord(1) << (p_i % 64);
	int64_t n = p_x.array.size();
	switch (p_b) {
	case 0:
		array = p_x.array;
		if (j >= n) {
			// no need to grow
			return;
		}
		array[j] &= ~m;
		norm();
		return;
	case 1:
		array = p_x.array;
		if (j >= n) {
			array.resize(j + 1);
		}
		array[j] |= m;
		// no need to normalize
		return;
	default:
		CRASH_NOW_MSG("set bit is not 0 or 1");
	}
}

// bit returns the value of the i'th bit, with lsb == bit 0.
uint64_t BigNat::bit(uint64_t p_i) const {
	int64_t j = p_i / 64;
	if (j >= array.size()) {
		return 0;
	}
	// 0 <= j < len(x)
	return (uint64_t(array[j]) >> (p_i % 64)) & 1;
}

// sticky returns 1 if there's a 1 bit within the
// i least significant bits, otherwise it returns 0.
uint64_t BigNat::sticky(uint64_t p_i) const {
	int64_t j = p_i / 64;
	if (j >= array.size()) {
		if (array.is_empty()) {
			return 0;
		}

		return 1;
	}

	// 0 <= j < len(x)
	for (int64_t i = 0; i < j; i++) {
		if (array[i] != 0) {
			return 1;
		}
	}

	if ((uint64_t(array[j]) << (64 - p_i % 64)) != 0) {
		return 1;
	}

	return 0;
}

void BigNat::and_(BigNat p_x, BigNat p_y) {
	int64_t m = p_x.array.size();
	int64_t n = p_y.array.size();
	if (m > n) {
		m = n;
	}
	// m <= n

	array.resize(m);
	for (int64_t i = 0; i < m; i++) {
		array[i] = p_x[i] & p_y[i];
	}

	norm();
}

// trunc returns z = x mod 2ⁿ.
void BigNat::trunc(BigNat p_x, uint64_t p_n) {
	const uint64_t w = (p_n + 64 - 1) / 64;
	if (p_x.array.size() < w) {
		set(p_x);
		return;
	}

	array = p_x.array.slice(0, w);

	if (p_n % 64 != 0) {
		(*this)[array.size() - 1] &= (1 << (p_n % 64)) - 1;
	}

	norm();
}

void BigNat::andNot(BigNat p_x, BigNat p_y) {
	int64_t m = p_x.array.size();
	int64_t n = p_y.array.size();
	if (n > m) {
		n = m;
	}
	// m >= n

	array = p_x.array.slice(0, m);
	for (int64_t i = 0; i < n; i++) {
		array[i] &= ~p_y[i];
	}

	norm();
}

void BigNat::or_(BigNat p_x, BigNat p_y) {
	if (p_x.array.size() >= p_y.array.size()) {
		array = p_x.array;
		for (int64_t i = 0; i < p_y.array.size(); i++) {
			array[i] |= p_y.array[i];
		}
	} else {
		array = p_y.array;
		for (int64_t i = 0; i < p_x.array.size(); i++) {
			array[i] |= p_x.array[i];
		}
	}

	norm();
}

void BigNat::xor_(BigNat p_x, BigNat p_y) {
	if (p_x.array.size() >= p_y.array.size()) {
		array = p_x.array;
		for (int64_t i = 0; i < p_y.array.size(); i++) {
			array[i] ^= p_y.array[i];
		}
	} else {
		array = p_y.array;
		for (int64_t i = 0; i < p_x.array.size(); i++) {
			array[i] ^= p_x.array[i];
		}
	}

	norm();
}

// random creates a random integer in [0..limit), using the space in z if
// possible. n is the bit length of limit.
void BigNat::random(const std::function<uint32_t()> &p_rnd, BigNat p_limit, int64_t p_n) {
	array.resize(p_limit.array.size());

	uint64_t bitLengthOfMSW = uint(p_n % 64);
	if (bitLengthOfMSW == 0) {
		bitLengthOfMSW = 64;
	}
	const uint64_t mask = (BigWord(1) << bitLengthOfMSW) - 1;

	while (true) {
		for (int64_t i = 0; i < array.size(); i++) {
			(*this)[i] = BigWord(p_rnd()) | (BigWord(p_rnd()) << 32);
		}

		(*this)[p_limit.array.size() - 1] &= mask;
		if (cmp(p_limit) < 0) {
			break;
		}
	}

	norm();
}

// If m != 0 (i.e., len(m) != 0), expNN sets z to x**y mod m;
// otherwise it sets z to x**y. The result is the value of z.
// The caller may pass stk == nil to request that expNN obtain and release one itself.
void BigNat::expNN(BigNat x, BigNat y, BigNat m, bool slow) {
	// x**y mod 1 == 0
	if (m.array.size() == 1 && m[0] == 1) {
		setUint64(0);
		return;
	}
	// m == 0 || m > 1

	// x**0 == 1
	if (y.array.is_empty()) {
		setUint64(1);
		return;
	}
	// y > 0

	// 0**y = 0
	if (x.array.is_empty()) {
		setUint64(0);
		return;
	}
	// x > 0

	// 1**y = 1
	if (x.array.size() == 1 && x[0] == 1) {
		setUint64(1);
		return;
	}
	// x > 1

	// x**1 == x
	if (y.array.size() == 1 && y[0] == 1 && m.array.is_empty()) {
		set(x);
		return;
	}

	if (y.array.size() == 1 && y[0] == 1) { // len(m) > 0
		rem(x, m);
		return;
	}

	// y > 1

	if (!m.array.is_empty()) {
		// We likely end up being as long as the modulus.
		array.resize(m.array.size());

		// If the exponent is large, we use the Montgomery method for odd values,
		// and a 4-bit, windowed exponentiation for powers of two,
		// and a CRT-decomposed Montgomery method for the remaining values
		// (even values times non-trivial odd values, which decompose into one
		// instance of each of the first two cases).
		if (y.array.size() > 1 && !slow) {
			if ((m[0] & 1) == 1) {
				expNNMontgomery(x, y, m);
				return;
			}

			const Pair<uint64_t, bool> logM = m.isPow2();
			if (logM.second) {
				expNNWindowed(x, y, logM.first);
				return;
			}

			expNNMontgomeryEven(x, y, m);
			return;
		}
	}

	BigNat &z = *this;

	z.set(x);
	BigWord v = y[y.array.size() - 1]; // v > 0 because y is normalized and y > 0
	uint64_t shift = std::countl_zero(v) + 1;
	v <<= shift;
	BigNat q;

	// We walk through the bits of the exponent one by one. Each time we
	// see a bit, we square, thus doubling the power. If the bit is a one,
	// we also multiply by x, thus adding one to the power.

	int64_t w = 64 - shift;
	// zz and r are used to avoid allocating in mul and div as
	// otherwise the arguments would alias.
	BigNat zz, r;
	for (int64_t j = 0; j < w; j++) {
		zz.sqr(z);
		SWAP(zz, z);

		if (v != 0) {
			zz.mul(z, x);
			SWAP(zz, z);
		}

		if (!m.array.is_empty()) {
			zz.div(r, z, m);
			std::tie(zz, r, q, z) = std::make_tuple(q, z, zz, r);
		}

		v <<= 1;
	}

	for (int64_t i = y.array.size() - 2; i >= 0; i--) {
		v = y[i];

		for (int64_t j = 0; j < 64; j++) {
			zz.sqr(z);
			SWAP(zz, z);

			if (v != 0) {
				zz.mul(z, x);
				SWAP(zz, z);
			}

			if (!m.array.is_empty()) {
				zz.div(r, z, m);
				std::tie(zz, r, q, z) = std::make_tuple(q, z, zz, r);
			}

			v <<= 1;
		}
	}

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
void BigNat::expNNMontgomeryEven(BigNat x, BigNat y, BigNat m) {
	// Split m = m₁ × m₂ where m₁ = 2ⁿ
	uint64_t n = m.trailingZeroBits();
	BigNat m1, m2;
	m1.lsh(BigNat{{1}}, n);
	m2.rsh(m, n);

	// We want z = x**y mod m.
	// z₁ = x**y mod m1 = (x**y mod m) mod m1 = z mod m1
	// z₂ = x**y mod m2 = (x**y mod m) mod m2 = z mod m2
	// (We are using the math/big convention for names here,
	// where the computation is z = x**y mod m, so its parts are z1 and z2.
	// The paper is computing x = a**e mod n; it refers to these as x2 and z1.)
	BigNat z1, z2;
	z1.expNN(x, y, m1, false);
	z2.expNN(x, y, m2, false);

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
	set(z2);

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
void BigNat::expNNWindowed(BigNat x, BigNat y, uint64_t logM) {
	CRASH_COND(y.array.size() <= 1);
	if ((x[0] & 1) == 0) {
		// len(y) > 1, so y  > logM.
		// x is even, so x**y is a multiple of 2**y which is a multiple of 2**logM.
		setUint64(0);
		return;
	}
	if (logM == 1) {
		setUint64(1);
		return;
	}

	// zz is used to avoid allocating in mul as otherwise
	// the arguments would alias.
	const int64_t w = int64_t((logM + 64 - 1) / 64);
	BigNat zz;

	constexpr int64_t n = 4;
	// powers[i] contains x^i.
	BigNat powers[1 << n];
	powers[0].setUint64(1);
	powers[1].trunc(x, logM);
	for (int64_t i = 2; i < (1 << n); i += 2) {
		BigNat &p2 = powers[i / 2];
		BigNat &p = powers[i];
		BigNat &p1 = powers[i + 1];
		p.sqr(p2);
		p.trunc(p, logM);
		p1.mul(p, x);
		p1.trunc(p1, logM);
	}

	// Because phi(2**logM) = 2**(logM-1), x**(2**(logM-1)) = 1,
	// so we can compute x**(y mod 2**(logM-1)) instead of x**y.
	// That is, we can throw away all but the bottom logM-1 bits of y.
	// Instead of allocating a new y, we start reading y at the right word
	// and truncate it appropriately at the start of the loop.
	int64_t i = y.array.size() - 1;
	int64_t mtop = int64_t((logM - 2) / 64); // -2 because the top word of N bits is the (N-1)/W'th word.
	BigWord mmask = UINT64_MAX;
	uint64_t mbits = (logM - 1) & (64 - 1);
	if (mbits != 0) {
		mmask = (1LLU << mbits) - 1;
	}
	if (i > mtop) {
		i = mtop;
	}
	bool advance = false;
	setUint64(1);
	for (; i >= 0; i--) {
		BigWord yi = y[i];
		if (i == mtop) {
			yi &= mmask;
		}

		for (int64_t j = 0; j < 64; j += n) {
			if (advance) {
				// Account for use of 4 bits in previous iteration.
				// Unrolled loop for significant performance
				// gain. Use go test -bench=".*" in crypto/rsa
				// to check performance before making changes.
				zz.sqr(*this);
				trunc(zz, logM);

				zz.sqr(*this);
				trunc(zz, logM);

				zz.sqr(*this);
				trunc(zz, logM);

				zz.sqr(*this);
				trunc(zz, logM);
			}

			zz.mul(*this, powers[yi >> (64 - n)]);
			trunc(zz, logM);

			yi <<= n;
			advance = true;
		}
	}

	norm();
}

// expNNMontgomery calculates x**y mod m using a fixed, 4-bit window.
// Uses Montgomery representation.
void BigNat::expNNMontgomery(BigNat x, BigNat y, BigNat m) {
	const int64_t numWords = m.array.size();

	// We want the lengths of x and m to be equal.
	// It is OK if x >= m as long as len(x) == len(m).
	if (x.array.size() > numWords) {
		x.rem(x, m);
		// Note: now len(x) <= numWords, not guaranteed ==.
	}
	if (x.array.size() < numWords) {
		x.array.resize(numWords);
	}

	// Ideally the precomputations would be performed outside, and reused
	// k0 = -m**-1 mod 2**_W. Algorithm from: Dumas, J.G. "On Newton–Raphson
	// Iteration for Multiplicative Inverses Modulo Prime Powers".
	BigWord k0 = 2 - m[0];
	BigWord t = m[0] - 1;
	for (int64_t i = 1; i < 64; i <<= 1) {
		t *= t;
		k0 *= t + 1;
	}
	k0 = -k0;

	// RR = 2**(2*_W*len(m)) mod m
	BigNat RR, zz;
	RR.setUint64(1);
	zz.lsh(RR, uint64_t(2 * numWords * 64));
	RR.rem(zz, m);
	if (RR.array.size() < numWords) {
		RR.array.resize(numWords);
	}
	// one = 1, with equal length to that of m
	BigNat one;
	one.array.resize(numWords);
	one[0] = 1;

	constexpr int64_t n = 4;
	// powers[i] contains x^i
	BigNat powers[1 << n];
	powers[0].montgomery(one, RR, m, k0, numWords);
	powers[1].montgomery(x, RR, m, k0, numWords);
	for (int64_t i = 2; i < (1 << n); i++) {
		powers[i].montgomery(powers[i - 1], powers[1], m, k0, numWords);
	}

	// initialize z = 1 (Montgomery 1)
	*this = powers[0];

	zz.array.resize(numWords);

	BigNat &z = *this;

	// same windowed exponent, but with Montgomery multiplications
	for (int64_t i = y.array.size() - 1; i >= 0; i--) {
		BigWord yi = y[i];
		for (int64_t j = 0; j < 64; j += n) {
			if (i != y.array.size() - 1 || j != 0) {
				zz.montgomery(z, z, m, k0, numWords);
				z.montgomery(zz, zz, m, k0, numWords);
				zz.montgomery(z, z, m, k0, numWords);
				z.montgomery(zz, zz, m, k0, numWords);
			}
			zz.montgomery(z, powers[yi >> (64 - n)], m, k0, numWords);
			SWAP(z, zz);
			yi <<= n;
		}
	}
	// convert to regular number
	zz.montgomery(z, one, m, k0, numWords);

	// One last reduction, just in case.
	// See golang.org/issue/13907.
	if (zz.cmp(m) >= 0) {
		// Common case is m has high bit set; in that case,
		// since zz is the same length as m, there can be just
		// one multiple of m to remove. Just subtract.
		// We think that the subtract should be sufficient in general,
		// so do that unconditionally, but double-check,
		// in case our beliefs are wrong.
		// The div is not expected to be reached.
		zz.sub(zz, m);
		if (zz.cmp(m) >= 0) {
			zz.rem(zz, m);
		}
	}

	*this = zz;
	norm();
}

// bytes writes the value of z into buf using big-endian encoding.
// The value of z is encoded in the slice buf[i:]. If the value of z
// cannot be represented in buf, bytes panics. The number i of unused
// bytes at the beginning of buf is returned as result.
int64_t BigNat::bytes(PackedByteArray &r_buf) const {
	int64_t i = r_buf.size();
	for (const int64_t d_ : array) {
		BigWord d = d_;
		for (int j = 0; j < 8; j++) {
			i--;
			if (i >= 0) {
				r_buf[i] = uint8_t(d);
			} else {
				CRASH_COND(uint8_t(d) != 0);
			}
			d >>= 8;
		}
	}

	if (i < 0) {
		i = 0;
	}
	while (i < r_buf.size() && r_buf[i] == 0) {
		i++;
	}

	return i;
}

// setBytes interprets buf as the bytes of a big-endian unsigned
// integer, sets z to that value, and returns z.
void BigNat::setBytes(const PackedByteArray &p_buf) {
	array.resize((p_buf.size() + 8 - 1) / 8);

	int64_t i = p_buf.size();
	for (int64_t k = 0; i >= 8; k++) {
		array[k] = BSWAP64(p_buf.decode_u64(i - 8));
		i -= 8;
	}

	if (i > 0) {
		BigWord d = 0;
		for (uint64_t s = 0; i > 0; s += 8) {
			d |= BigWord(p_buf[i - 1]) << s;
			i--;
		}
		array[array.size() - 1] = d;
	}

	norm();
}

// sqrt sets z = ⌊√x⌋
// The caller may pass stk == nil to request that sqrt obtain and release one itself.
void BigNat::sqrt(BigNat x) {
	if (x.cmp(BigNat{{1}}) <= 0) {
		set(x);
		return;
	}

	// Start with value known to be too large and repeat "z = ⌊(z + ⌊x/z⌋)/2⌋" until it stops getting smaller.
	// See Brent and Zimmermann, Modern Computer Arithmetic, Algorithm 1.13 (SqrtInt).
	// https://members.loria.fr/PZimmermann/mca/pub226.html
	// If x is one less than a perfect square, the sequence oscillates between the correct z and z+1;
	// otherwise it converges to the correct z and stays there.
	BigNat z1, z2, r;
	z1.setUint64(1);
	z1.lsh(z1, uint64_t(x.bitLen() + 1) / 2); // must be ≥ √x
	for (int64_t n = 0; ; n++) {
		z2.div(r, x, z1);
		z2.add(z2, z1);
		z2.rsh(z2, 1);
		if (z2.cmp(z1) >= 0) {
			// z1 is answer.
			set(z1);
			return;
		}
		SWAP(z1, z2);
	}
}

// subMod2N returns z = (x - y) mod 2ⁿ.
void BigNat::subMod2N(BigNat x, BigNat y, uint64_t n) {
	if (x.bitLen() > n) {
		x.trunc(x, n);
	}
	if (y.bitLen() > n) {
		y.trunc(y, n);
	}
	if (x.cmp(y) >= 0) {
		sub(x, y);
		return;
	}
	// x - y < 0; x - y mod 2ⁿ = x - y + 2ⁿ = 2ⁿ - (y - x) = 1 + 2ⁿ-1 - (y - x) = 1 + ^(y - x).
	sub(y, x);
	while (array.size() * 64 < n) {
		array.append(0);
	}
	for (int64_t i = 0; i < array.size(); i++) {
		array[i] = ~array[i];
	}
	trunc(*this, n);
	add(*this, BigNat{{1}});
}
