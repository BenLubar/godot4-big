// This file is ported from src/math/big/decimal.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements multi-precision decimal numbers.
// The implementation is for float to decimal conversion only;
// not general purpose use.
// The only operations are precise conversion from binary to
// decimal and rounding.
//
// The key observation and some code (shr) is borrowed from
// strconv/decimal.go: conversion of binary fractional values can be done
// precisely in multi-precision decimal because 2 divides 10 (required for
// >> of mantissa); but conversion of decimal floating-point values cannot
// be done precisely in binary representation.
//
// In contrast to strconv/decimal.go, only right shift is implemented in
// decimal format - left shift can be done precisely in binary format.

#pragma once

#include "godot_big_naturals.h"

class BigFloat;

// A decimal represents an unsigned floating-point number in decimal representation.
// The value of a non-zero decimal d is d.mant * 10**d.exp with 0.1 <= d.mant < 1,
// with the most-significant mantissa digit at index 0. For the zero decimal, the
// mantissa length and exponent are 0.
// The zero value for decimal represents a ready-to-use 0.0.
struct BigDecimal {
	godot::PackedByteArray mant; // mantissa ASCII digits, big-endian
	int64_t exp = 0; // exponent

	// at returns the i'th mantissa digit, starting with the most significant digit at 0.
	[[nodiscard]] inline char at(int64_t i) const {
		if (0 <= i && i < mant.size()) {
			return mant[i];
		}
		return '0';
	}

	// Maximum shift amount that can be done in one pass without overflow.
	// A Word has _W bits and (1<<maxShift - 1)*10 + 9 must fit into Word.
	static constexpr int64_t maxShift = 64 - 4;

	// TODO(gri) Since we know the desired decimal precision when converting
	// a floating-point number, we may be able to limit the number of decimal
	// digits that need to be computed by init by providing an additional
	// precision argument and keeping track of when a number was truncated early
	// (equivalent of "sticky bit" in binary rounding).

	// TODO(gri) Along the same lines, enforce some limit to shift magnitudes
	// to avoid "infinitely" long running conversions (until we run out of space).

	// Init initializes x to the decimal representation of m << shift (for
	// shift >= 0), or m >> -shift (for shift < 0).
	inline void init(BigNat m, int64_t shift) {
		// special case 0
		if (m.array.is_empty()) {
			mant.clear();
			exp = 0;
			return;
		}

		// Optimization: If we need to shift right, first remove any trailing
		// zero bits from m to reduce shift amount that needs to be done in
		// decimal format (since that is likely slower).
		if (shift < 0) {
			const uint64_t ntz = m.trailingZeroBits();
			const uint64_t s = godot::Math::min(static_cast<uint64_t>(-shift), ntz); // shift at most ntz bits
			m.rsh(m, s);
			shift += static_cast<int64_t>(s);
		}

		// Do any shift left in binary representation.
		if (shift > 0) {
			m.lsh(m, static_cast<uint64_t>(shift));
			shift = 0;
		}

		// Convert mantissa into decimal representation.
		const godot::String s = m.utoa(10);
		int64_t n = s.length();
		exp = n;
		// Trim trailing zeros; instead the exponent is tracking
		// the decimal point independent of the number of digits.
		while (n > 0 && s[n - 1] == '0') {
			n--;
		}
		mant = s.substr(0, n).to_ascii_buffer();

		// Do any (remaining) shift right in decimal representation.
		if (shift < 0) {
			while (shift < -maxShift) {
				rsh(maxShift);
				shift += maxShift;
			}

			rsh(static_cast<uint64_t>(-shift));
		}
	}

	// rsh implements x >> s, for s <= maxShift.
	inline void rsh(uint64_t s) {
		// Division by 1<<s using shift-and-subtract algorithm.

		// pick up enough leading digits to cover first shift
		int64_t r = 0; // read index
		BigWord n = 0;
		while ((n >> s) == 0 && r < mant.size()) {
			const BigWord ch = static_cast<BigWord>(mant[r]);
			r++;
			n = (n * 10) + ch - '0';
		}
		if (n == 0) {
			// x == 0; shouldn't get here, but handle anyway
			mant.clear();
			return;
		}
		while ((n >> s) == 0) {
			r++;
			n *= 10;
		}

		exp += 1 - r;

		// read a digit, write a digit
		int64_t w = 0; // write index
		const BigWord mask = (static_cast<BigWord>(1) << s) - 1;
		while (r < mant.size()) {
			const BigWord ch = static_cast<BigWord>(mant[r]);
			r++;
			const BigWord d = n >> s;
			n &= mask; // n -= d << s
			mant[w] = static_cast<uint8_t>(d + '0');
			w++;
			n = (n * 10) + ch - '0';
		}

		// write extra digits that still fit
		while (n > 0 && w < mant.size()) {
			const BigWord d = n >> s;
			n &= mask;
			mant[w] = static_cast<uint8_t>(d + '0');
			w++;
			n *= 10;
		}
		mant.resize(w); // the number may be shorter (e.g. 1024 >> 10)

		// append additional digits that didn't fit
		while (n > 0) {
			const BigWord d = n >> s;
			n &= mask;
			mant.append(static_cast<uint8_t>(d + '0'));
			n *= 10;
		}

		trim();
	}

	// shouldRoundUp reports if x should be rounded up
	// if shortened to n digits. n must be a valid index
	// for x.mant.
	[[nodiscard]] inline bool shouldRoundUp(int64_t n) const {
		if (mant[n] == '5' && n + 1 == mant.size()) {
			// exactly halfway - round to even
			return n > 0 && ((mant[n - 1] - '0') & 1) != 0;
		}

		// not halfway - digit tells all (x.mant has no trailing zeros)
		return mant[n] >= '5';
	}

	// round sets x to (at most) n mantissa digits by rounding it
	// to the nearest even value with n (or fever) mantissa digits.
	// If n < 0, x remains unchanged.
	inline void round(int64_t n) {
		if (n < 0 || n >= mant.size()) {
			return; // nothing to do
		}

		if (shouldRoundUp(n)) {
			roundUp(n);
		} else {
			roundDown(n);
		}
	}

	inline void roundUp(int64_t n) {
		if (n < 0 || n >= mant.size()) {
			return; // nothing to do
		}
		// 0 <= n < len(x.mant)

		// find first digit < '9'
		while (n > 0 && mant[n - 1] >= '9') {
			n--;
		}

		if (n == 0) {
			// all digits are '9's => round up to '1' and update exponent
			mant[0] = '1'; // ok since len(x.mant) > n
			mant.resize(1);
			exp++;
			return;
		}

		// n > 0 && x.mant[n-1] < '9'
		mant[n - 1]++;
		mant.resize(n);
		// x already trimmed
	}

	inline void roundDown(int64_t n) {
		if (n < 0 || n >= mant.size()) {
			return; // nothing to do
		}
		mant.resize(n);
		trim();
	}

	// trim cuts off any trailing zeros from x's mantissa;
	// they are meaningless for the value of x.
	inline void trim() {
		int64_t i = mant.size();
		while (i > 0 && mant[i - 1] == '0') {
			i--;
		}
		mant.resize(i);
		if (i == 0) {
			exp = 0;
		}
	}

	void roundShortest(const BigFloat *p_x);
};
