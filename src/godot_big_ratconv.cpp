// This file is ported from src/math/big/ratconv.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements rat-to-string conversion functions.

#include "godot_big_rat.h"
#include "godot_big_int.h"

using namespace godot;

// SetString sets z to the value of s and returns z and a boolean indicating
// success. s can be given as a (possibly signed) fraction "a/b", or as a
// floating-point number optionally followed by an exponent.
// If a fraction is provided, both the dividend and the divisor may be a
// decimal integer or independently use a prefix of “0b”, “0” or “0o”,
// or “0x” (or their upper-case variants) to denote a binary, octal, or
// hexadecimal integer, respectively. The divisor may not be signed.
// If a floating-point number is provided, it may be in decimal form or
// use any of the same prefixes as above but for “0” to denote a non-decimal
// mantissa. A leading “0” is considered a decimal leading 0; it does not
// indicate octal representation in this case.
// An optional base-10 “e” or base-2 “p” (or their upper-case variants)
// exponent may be provided as well, except for hexadecimal floats which
// only accept an (optional) “p” exponent (because an “e” or “E” cannot
// be distinguished from a mantissa digit). If the exponent's absolute value
// is too large, the operation may fail.
// The entire string, not just a prefix, must be valid for success. If the
// operation failed, the value of z is undefined but the returned value is nil.
Error BigRat::SetString(const godot::String &p_s) {
	ERR_FAIL_COND_V(p_s.is_empty(), ERR_INVALID_PARAMETER);
	// len(s) > 0

	// parse fraction a/b, if any
	int64_t sep = p_s.find("/");
	if (sep >= 0) {
		Ref<BigInt> n, d;
		n.instantiate();
		d.instantiate();

		Error err = n->SetString(p_s.substr(0, sep), 0);
		if (err != OK) {
			return err;
		}

		err = d->SetString(p_s.substr(sep + 1), 0);
		if (err != OK) {
			return err;
		}

		ERR_FAIL_COND_V(d->_neg, ERR_INVALID_PARAMETER);
		ERR_FAIL_COND_V(d->_abs.array.is_empty(), ERR_INVALID_PARAMETER);

		return SetFrac(n, d);
	}

	// parse floating-point number
	int64_t off = 0;

	// sign
	bool neg;
	Error err = BigNat::scanSign(p_s, off, neg);
	if (err != OK) {
		return err;
	}

	// mantissa
	BigNat a;
	int64_t base = 0;
	int64_t fcount = 0; // fractional digit count; valid if <= 0
	err = a.scan(p_s, off, 0, true, base, fcount);
	if (err != OK) {
		return err;
	}

	// exponent
	int64_t exp = 0;
	int64_t ebase = 0;
	err = BigNat::scanExponent(p_s, off, true, true, exp, ebase);
	if (err != OK) {
		return err;
	}

	// there should be no unread characters left
	ERR_FAIL_COND_V(p_s.length() != off, ERR_INVALID_PARAMETER);

	// special-case 0 (see also issue #16176)
	if (a.array.is_empty()) {
		_neg = false;
		_a.array.clear();
		_b.setUint64(1);
		emit_changed();
		return OK;
	}
	// len(z.a.abs) > 0

	// The mantissa may have a radix point (fcount <= 0) and there
	// may be a nonzero exponent exp. The radix point amounts to a
	// division by base**(-fcount), which equals a multiplication by
	// base**fcount. An exponent means multiplication by ebase**exp.
	// Multiplications are commutative, so we can apply them in any
	// order. We only have powers of 2 and 10, and we split powers
	// of 10 into the product of the same powers of 2 and 5. This
	// may reduce the size of shift/multiplication factors or
	// divisors required to create the final fraction, depending
	// on the actual floating-point value.

	// determine binary or decimal exponent contribution of radix point
	int64_t exp2 = 0, exp5 = 0;
	if (fcount < 0) {
		// The mantissa has a radix point ddd.dddd; and
		// -fcount is the number of digits to the right
		// of '.'. Adjust relevant exponent accordingly.
		switch (base) {
		case 10:
			exp5 = fcount;
			[[fallthrough]]; // 10**e == 5**e * 2**e
		case 2:
			exp2 = fcount;
			break;
		case 8:
			exp2 = fcount * 3; // octal digits are 3 bits each
			break;
		case 16:
			exp2 = fcount * 4; // hexadecimal digits are 4 bits each
			break;
		default:
			CRASH_NOW_MSG("unexpected mantissa base");
		}
		// fcount consumed - not needed anymore
	}

	// take actual exponent into account
	switch (ebase) {
	case 10:
		exp5 += exp;
		[[fallthrough]]; // see fallthrough above
	case 2:
		exp2 += exp;
		break;
	default:
		CRASH_NOW_MSG("unexpected exponent base");
	}
	// exp consumed - not needed anymore

	// apply exp5 contributions
	// (start with exp5 so the numbers to multiply are smaller)
	BigNat b;
	if (exp5 != 0) {
		int64_t n = exp5;
		if (n < 0) {
			n = -n;
			// This can occur if -n overflows. -(-1 << 63) would become
			// -1 << 63, which is still negative.
			ERR_FAIL_COND_V(n < 0, ERR_PARAMETER_RANGE_ERROR);
		}
		ERR_FAIL_COND_V(n > 1e6, ERR_PARAMETER_RANGE_ERROR); // avoid excessively large exponents
		b.expWW(5, n); // use underlying array of z.b.abs
		if (exp5 > 0) {
			a.mul(a, b);
			b.setUint64(1);
		}
	} else {
		b.setUint64(1);
	}

	// apply exp2 contributions
	ERR_FAIL_COND_V(exp2 < -1e7 || exp2 > 1e7, ERR_PARAMETER_RANGE_ERROR); // avoid excessively large exponents

	_a.set(a);
	_b.set(b);

	if (exp2 > 0) {
		_a.lsh(_a, uint64_t(exp2));
	} else if (exp2 < 0) {
		_b.lsh(_b, uint64_t(-exp2));
	}

	_neg = neg && !_a.array.is_empty(); // 0 has no sign

	_norm();
	return OK;
}

// scanExponent scans the longest possible prefix of r representing a base 10
// (“e”, “E”) or a base 2 (“p”, “P”) exponent, if any. It returns the
// exponent, the exponent base (10 or 2), or a read or syntax error, if any.
//
// If sepOk is set, an underscore character “_” may appear between successive
// exponent digits; such underscores do not change the value of the exponent.
// Incorrect placement of underscores is reported as an error if there are no
// other errors. If sepOk is not set, underscores are not recognized and thus
// terminate scanning like any other character that is not a valid digit.
//
//	exponent = ( "e" | "E" | "p" | "P" ) [ sign ] digits .
//	sign     = "+" | "-" .
//	digits   = digit { [ '_' ] digit } .
//	digit    = "0" ... "9" .
//
// A base 2 exponent is only permitted if base2ok is set.
Error BigNat::scanExponent(const String &s, int64_t &off, bool base2ok, bool sepOk, int64_t &exp, int64_t &base) {
	// one char look-ahead
	if (s.length() <= off) {
		exp = 0;
		base = 10;
		return OK;
	}

	// exponent char
	switch (s[off]) {
	case 'e':
	case 'E':
		base = 10;
		break;
	case 'p':
	case 'P':
		if (base2ok) {
			base = 2;
			break; // ok
		}
		[[fallthrough]]; // binary exponent not permitted
	default:
		exp = 0;
		base = 10;
		return OK;
	}

	off++;

	// sign
	PackedByteArray digits;
	if (off < s.length() && (s[off] == '+' || s[off] == '-')) {
		if (s[off] == '-') {
			digits.append('-');
		}
		off++;
	}

	// prev encodes the previously seen char: it is one
	// of '_', '0' (a digit), or '.' (anything else). A
	// valid separator '_' may only occur after a digit.
	char prev = '.';
	bool invalSep = false;

	// exponent value
	bool hasDigits = false;
	while (off < s.length()) {
		if ('0' <= s[off] && s[off] <= '9') {
			digits.append(s[off]);
			prev = '0';
			hasDigits = true;
		} else if (s[off] == '_' && sepOk) {
			if (prev != '0') {
				invalSep = true;
			}
			prev = '_';
		} else {
			off--; // ch does not belong to number anymore
			break;
		}
		off++;
	}

	ERR_FAIL_COND_V(!hasDigits, ERR_INVALID_DATA);
	const String digitString = digits.get_string_from_ascii();
	ERR_FAIL_COND_V(!digitString.is_valid_int(), ERR_INVALID_DATA);
	exp = digitString.to_int();
	// other errors take precedence over invalid separators
	ERR_FAIL_COND_V(invalSep || prev == '_', ERR_INVALID_DATA);

	return OK;
}

// String returns a string representation of x in the form "a/b" (even if b == 1).
String BigRat::String() const {
	return _a.itoa(_neg, 10) + "/" + _b.utoa(10);
}

// RatString returns a string representation of x in the form "a/b" if b != 1,
// and in the form "a" if b == 1.
String BigRat::RatString() const {
	if (IsInt()) {
		return _a.itoa(_neg, 10);
	}
	return String();
}

// FloatString returns a string representation of x in decimal form with prec
// digits of precision after the radix point. The last digit is rounded to
// nearest, with halves rounded away from zero.
String BigRat::FloatString(int64_t p_prec) const {
	PackedByteArray buf;

	if (IsInt()) {
		godot::String s = _a.itoa(_neg, 10);
		if (p_prec > 0) {
			buf = s.to_ascii_buffer();
			buf.append('.');
			for (int64_t i = p_prec; i > 0; i--) {
				buf.append('0');
			}
			s = buf.get_string_from_ascii();
		}
		return s;
	}
	// x.b.abs != 0

	BigNat q, r, r2;
	q.div(r, _a, _b);

	BigNat p{{1}};
	if (p_prec > 0) {
		p.expWW(10, p_prec);
	}

	r.mul(r, p);
	r.div(r2, r, _b);

	// see if we need to round up
	r2.lsh(r2, 1);
	if (_b.cmp(r2) <= 0) {
		r.add(r, BigNat{{1}});
		if (r.cmp(p) >= 0) {
			q.add(q, BigNat{{1}});
			r.sub(r, p);
		}
	}

	if (_neg) {
		buf.append('-');
	}
	buf.append_array(q.utoa(10).to_ascii_buffer()); // itoa ignores sign if q == 0

	if (p_prec > 0) {
		buf.append('.');
		const godot::String rs = r.utoa(10);
		for (int64_t i = p_prec - rs.length(); i > 0; i--) {
			buf.append('0');
		}
		buf.append_array(rs.to_ascii_buffer());
	}

	return buf.get_string_from_ascii();
}

// Note: FloatPrec (below) is in this file rather than rat.go because
//       its results are relevant for decimal representation/printing.

// FloatPrec returns the number n of non-repeating digits immediately
// following the decimal point of the decimal representation of x.
// The boolean result indicates whether a decimal representation of x
// with that many fractional digits is exact or rounded.
//
// Examples:
//
//	x      n    exact    decimal representation n fractional digits
//	0      0    true     0
//	1      0    true     1
//	1/2    1    true     0.5
//	1/3    0    false    0       (0.333... rounded)
//	1/4    2    true     0.25
//	1/6    1    false    0.2     (0.166... rounded)
Pair<int64_t, bool> BigRat::FloatPrec() const {
	// Determine q and largest p2, p5 such that d = q·2^p2·5^p5.
	// The results n, exact are:
	//
	//     n = max(p2, p5)
	//     exact = q == 1
	//
	// For details see:
	// https://en.wikipedia.org/wiki/Repeating_decimal#Reciprocals_of_integers_not_coprime_to_10
	BigNat d = _b; // d >= 1

	// Determine p2 by counting factors of 2.
	// p2 corresponds to the trailing zero bits in d.
	// Do this first to reduce q as much as possible.
	BigNat q;
	const uint64_t p2 = d.trailingZeroBits();
	q.rsh(d, p2);

	// Determine p5 by counting factors of 5.
	// Build a table starting with an initial power of 5,
	// and use repeated squaring until the factor doesn't
	// divide q anymore. Then use the table to determine
	// the power of 5 in q.
	constexpr uint64_t fp = 13; // f == 5^fp
	LocalVector<BigNat> tab;   // tab[i] == (5^fp)^(2^i) == 5^(fp·2^i)
	BigNat f{{1220703125}};    // == 5^fp (must fit into a uint32 Word)
	BigNat t, r;               // temporaries
	while (true) {
		t.div(r, q, f);
		if (!r.array.is_empty()) {
			break; // f doesn't divide q evenly
		}
		tab.push_back(f);
		f.sqr(f);
	}

	// Factor q using the table entries, if any.
	// We start with the largest factor f = tab[len(tab)-1]
	// that evenly divides q. It does so at most once because
	// otherwise f·f would also divide q. That can't be true
	// because f·f is the next higher table entry, contradicting
	// how f was chosen in the first place.
	// The same reasoning applies to the subsequent factors.
	uint64_t p5 = 0;
	for (int64_t i = int64_t(tab.size()) - 1; i >= 0; i--) {
		t.div(r, q, tab[i]);
		if (r.array.is_empty()) {
			p5 += fp * (1LLU << i); // tab[i] == 5^(fp·2^i)
			q.set(t);
		}
	}

	// If fp != 1, we may still have multiples of 5 left.
	while (true) {
		t.div(r, q, BigNat{{5}});
		if (!r.array.is_empty()) {
			break;
		}
		p5++;
		q.set(t);
	}

	return {int64_t(Math::max(p2, p5)), q.cmp(BigNat{{1}}) == 0};
}
