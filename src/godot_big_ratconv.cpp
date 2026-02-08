// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_rat.h"
#include "godot_big_float.h"
#include "godot_big_naturals.h"

// This file implements rat-to-string conversion functions.

/*
func ratTok(ch rune) bool {
	return strings.ContainsRune("+-/0123456789.eE", ch)
}
*/

/*
var ratZero Rat
var _ fmt.Scanner = &ratZero // *Rat must implement fmt.Scanner
*/

/*
// Scan is a support routine for fmt.Scanner. It accepts the formats
// 'e', 'E', 'f', 'F', 'g', 'G', and 'v'. All formats are equivalent.
func (z *Rat) Scan(s fmt.ScanState, ch rune) error {
	tok, err := s.Token(true, ratTok)
	if err != nil {
		return err
	}
	if !strings.ContainsRune("efgEFGv", ch) {
		return errors.New("Rat.Scan: invalid verb")
	}
	if _, ok := z.SetString(string(tok)); !ok {
		return errors.New("Rat.Scan: invalid syntax")
	}
	return nil
}
*/

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
Ref<BigRat> BigRat::SetString(const godot::String &s) {
	ERR_FAIL_COND_V(s.is_empty(), nullptr);

	// len(s) > 0

	// parse fraction a/b, if any
	int64_t sep = s.find("/");
	if (sep >= 0) {
		Ref<BigInt> a, b;
		a.instantiate();
		b.instantiate();
		ERR_FAIL_COND_V(a->SetString(s.substr(0, sep), 0).is_null(), nullptr);
		ERR_FAIL_COND_V(b->SetString(s.substr(sep + 1), 0).is_null(), nullptr);
		ERR_FAIL_COND_V(b->Cmp(*intOne) < 0, nullptr);
		_a->Set(a);
		_b->Set(b);
		return norm();
	}

	// Go does a whole bunch of juggling here for performance, but I'm just gonna make a float and convert it back to a rat.
	Ref<BigFloat> f;
	f.instantiate();
	ERR_FAIL_COND_V(f->SetString(s).is_null(), nullptr);
	ERR_FAIL_COND_V(f->IsInf(), nullptr);
	f->Rat(this);

	return this;
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
bool nat_scanExponent(const PackedByteArray &buf, int64_t &i, bool base2ok, bool sepOk, int64_t &exp, int64_t &ebase) {
	// one char look-ahead
	if (i >= buf.size()) {
		exp = 0;
		ebase = 10;
		return true;
	}

	char ch = buf[i++];

	// exponent char
	if (ch == 'e' || ch == 'E') {
		ebase = 10;
	} else if (ch == 'p' || ch == 'P') {
		if (base2ok) {
			ebase = 2; // ok
		} else {
			// binary exponent not permitted
			i--;
			exp = 0;
			ebase = 10;
			return true;
		}
	} else {
		// ch does not belong to exponent anymore
		i--;
		exp = 0;
		ebase = 10;
		return true;
	}

	// sign
	PackedByteArray digits;
	if (i < buf.size() && (buf[i] == '+' || buf[i] == '-')) {
		if (buf[i] == '-') {
			digits.append('-');
		}

		i++;
	}

	// prev encodes the previously seen char: it is one
	// of '_', '0' (a digit), or '.' (anything else). A
	// valid separator '_' may only occur after a digit.
	char prev = '.';
	bool invalSep = false;

	// exponent value
	bool hasDigits = false;
	while (i < buf.size()) {
		ch = buf[i++];

		if ('0' <= ch && ch <= '9') {
			digits.append(ch);
			prev = '0';
			hasDigits = true;
		} else if (ch == '_' && sepOk) {
			if (prev != '0') {
				invalSep = true;
			}
			prev = '_';
		} else {
			i--; // ch does not belong to number anymore
			break;
		}
	}

	ERR_FAIL_COND_V_MSG(!hasDigits, false, "no digits in exponent");

	// other errors take precedence over invalid separators
	ERR_FAIL_COND_V_MSG(invalSep || prev == '_', false, "invalid separator in exponent");

	exp = digits.get_string_from_ascii().to_int();

	return true;
}

// String returns a string representation of x in the form "a/b" (even if b == 1).
String BigRat::String() const {
	if (_b->_abs.is_empty()) {
		return vformat("%s/1", _a->Text(10));
	}

	return vformat("%s/%s", _a->Text(10), _b->Text(10));
}

// RatString returns a string representation of x in the form "a/b" if b != 1,
// and in the form "a" if b == 1.
String BigRat::RatString() const {
	if (IsInt()) {
		return _a->Text(10);
	}

	return String();
}

// FloatString returns a string representation of x in decimal form with prec
// digits of precision after the radix point. The last digit is rounded to
// nearest, with halves rounded away from zero.
String BigRat::FloatString(int64_t prec) const {
	if (IsInt()) {
		// ugh, shadowing
		godot::String s = _a->Text(10);

		if (prec > 0) {
			s += ".";
			s += godot::String("0").repeat(prec - 1);
		}

		return s;
	}

	// x.b.abs != 0

	PackedInt64Array q, r;
	nat_div(_a->_abs, _b->_abs, q, r);

	PackedInt64Array p = *natOne;
	if (prec > 0) {
		PackedInt64Array exp;
		nat_setUint64(exp, uint64_t(prec));
		nat_expNN(p, *natTen, exp, PackedInt64Array(), false);
	}

	nat_mul(r, r, p);
	PackedInt64Array r2;
	nat_div(r, _b->_abs, r, r2);

	// see if we need to round up
	nat_lsh(r2, r2, 1);
	if (nat_cmp(_b->_abs, r2) <= 0) {
		nat_add(r, r, *natOne);
		if (nat_cmp(r, p) >= 0) {
			nat_add(q, q, *natOne);
			nat_add(r, r, p);
		}
	}

	PackedByteArray buf;
	if (_a->_neg) {
		buf.append('-');
	}
	buf.append_array(nat_utoa(q, 10)); // itoa ignores sign if q == 0

	if (prec > 0) {
		buf.append('.');
		const PackedByteArray rs = nat_utoa(r, 10);
		for (int64_t i = prec - rs.size(); i > 0; i--) {
			buf.append('0');
		}
		buf.append_array(rs);
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
void BigRat::FloatPrec(int64_t &n, bool &exact) const {
	// Determine q and largest p2, p5 such that d = q·2^p2·5^p5.
	// The results n, exact are:
	//
	//     n = max(p2, p5)
	//     exact = q == 1
	//
	// For details see:
	// https://en.wikipedia.org/wiki/Repeating_decimal#Reciprocals_of_integers_not_coprime_to_10
	PackedInt64Array d = Denom()->_abs; // d >= 1

	// Determine p2 by counting factors of 2.
	// p2 corresponds to the trailing zero bits in d.
	// Do this first to reduce q as much as possible.
	PackedInt64Array q;
	const uint64_t p2 = nat_trailingZeroBits(d);
	nat_rsh(q, d, p2);

	// Determine p5 by counting factors of 5.
	// Build a table starting with an initial power of 5,
	// and use repeated squaring until the factor doesn't
	// divide q anymore. Then use the table to determine
	// the power of 5 in q.
	static constexpr uint64_t fp = 13; // f == 5^fp
	LocalVector<PackedInt64Array> tab; // tab[i] == (5^fp)^(2^i) == 5^(fp·2^i)
	PackedInt64Array f{1220703125LLU}; // == 5^fp (must fit into a uint32 Word)
	PackedInt64Array t, r;             // temporaries

	while (true) {
		nat_div(q, f, t, r);
		if (!r.is_empty()) {
			break; // f doesn't divide q evenly
		}

		tab.push_back(f);
		nat_sqr(f, f);
	}

	// Factor q using the table entries, if any.
	// We start with the largest factor f = tab[len(tab)-1]
	// that evenly divides q. It does so at most once because
	// otherwise f·f would also divide q. That can't be true
	// because f·f is the next higher table entry, contradicting
	// how f was chosen in the first place.
	// The same reasoning applies to the subsequent factors.
	uint64_t p5 = 0;
	for (int64_t i = tab.size() - 1; i >= 0; i--) {
		nat_div(q, tab[i], t, r);
		if (r.is_empty()) {
			p5 += fp * (1LLU << i); // tab[i] == 5^(fp·2^i)
			nat_set(q, t);
		}
	}

	// If fp != 1, we may still have multiples of 5 left.
	while (true) {
		nat_div(q, *natFive, t, r);
		if (!r.is_empty()) {
			break;
		}

		p5++;
		nat_set(q, t);
	}

	n = int64_t(Math::max(p2, p5));
	exact = nat_cmp(q, *natOne) == 0;
}
