// This file is ported from src/math/big/floatconv.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"

using namespace godot;

// This file implements string-to-Float conversion functions.

// scan is like Parse but reads the longest possible prefix representing a valid
// floating point number from an io.ByteScanner rather than a string. It serves
// as the implementation of Parse. It does not recognize ±Inf and does not expect
// EOF at the end.
Error BigFloat::_scan(const godot::String &s, int64_t &off, int64_t &base) {
	uint32_t prec = _prec;
	if (prec == 0) {
		prec = 64;
	}

	// A reasonable value in case of an error.
	_form = FORM_ZERO;

	// sign
	Error err = BigNat::scanSign(s, off, _neg);
	if (err != OK) {
		return err;
	}

	// mantissa
	const int64_t orig_base = base;
	int64_t fcount = 0; // fractional digit count; valid if <= 0
	err = _mant.scan(s, off, base, true, base, fcount);
	if (err != OK) {
		return err;
	}

	// exponent
	int64_t exp = 0;
	int64_t ebase = 0;
	err = BigNat::scanExponent(s, off, true, orig_base == 0, exp, ebase);
	if (err != OK) {
		return err;
	}

	// special-case 0
	if (_mant.array.is_empty()) {
		_prec = prec;
		_acc = ACC_EXACT;
		_form = FORM_ZERO;
		return OK;
	}
	// len(z.mant) > 0

	// The mantissa may have a radix point (fcount <= 0) and there
	// may be a nonzero exponent exp. The radix point amounts to a
	// division by b**(-fcount). An exponent means multiplication by
	// ebase**exp. Finally, mantissa normalization (shift left) requires
	// a correcting multiplication by 2**(-shiftcount). Multiplications
	// are commutative, so we can apply them in any order as long as there
	// is no loss of precision. We only have powers of 2 and 10, and
	// we split powers of 10 into the product of the same powers of
	// 2 and 5. This reduces the size of the multiplication factor
	// needed for base-10 exponents.

	// normalize mantissa and determine initial exponent contributions
	int64_t exp2 = (_mant.array.size() * 64) - _mant.fnorm();
	int64_t exp5 = 0;

	// determine binary or decimal exponent contribution of radix point
	if (fcount < 0) {
		// The mantissa has a radix point ddd.dddd; and
		// -fcount is the number of digits to the right
		// of '.'. Adjust relevant exponent accordingly.
		switch (base) {
			case 10:
				exp5 = fcount;
				[[fallthrough]]; // 10**e == 5**e * 2**e
			case 2:
				exp2 += fcount;
				break;
			case 8:
				exp2 += fcount * 3; // octal digits are 3 bits each
				break;
			case 16:
				exp2 += fcount * 4; // hexadecimal digits are 4 bits each
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

	// apply 2**exp2
	if (MIN_EXP <= exp2 && exp2 <= MAX_EXP) {
		_prec = prec;
		_form = FORM_FINITE;
		_exp = static_cast<int32_t>(exp2);
	} else {
		ERR_FAIL_V_MSG(ERR_INVALID_DATA, "exponent overflow");
	}

	if (exp5 == 0) {
		// no decimal exponent contribution
		_round(0);
		return OK;
	}
	// exp5 != 0

	// apply 5**exp5
	Ref<BigFloat> p{ memnew(BigFloat) };
	p->SetPrec(Prec() + 64); // use more bits for p -- TODO(gri) what is the right number?
	if (exp5 < 0) {
		p->_pow5(static_cast<uint64_t>(-exp5));
		Quo(this, p);
	} else {
		p->_pow5(static_cast<uint64_t>(exp5));
		Mul(this, p);
	}

	return OK;
}

// These powers of 5 fit into a uint64.
//
//	for p, q := uint64(0), uint64(1); p < q; p, q = q, q*5 {
//		fmt.Println(q)
//	}
static constexpr std::array<uint64_t, 28> pow5tab{
	1,
	5,
	25,
	125,
	625,
	3125,
	15625,
	78125,
	390625,
	1953125,
	9765625,
	48828125,
	244140625,
	1220703125,
	6103515625,
	30517578125,
	152587890625,
	762939453125,
	3814697265625,
	19073486328125,
	95367431640625,
	476837158203125,
	2384185791015625,
	11920928955078125,
	59604644775390625,
	298023223876953125,
	1490116119384765625,
	7450580596923828125,
};

// pow5 sets z to 5**n and returns z.
// n must not be negative.
void BigFloat::_pow5(uint64_t p_n) {
	constexpr uint64_t m = (sizeof(pow5tab) / sizeof(pow5tab[0])) - 1;
	if (p_n <= m) {
		SetUint64(pow5tab[p_n]);
		return;
	}
	// n > m

	SetUint64(pow5tab[m]);
	p_n -= m;

	// use more bits for f than for z
	// TODO(gri) what is the right number?
	Ref<BigFloat> f;
	f->SetPrec(Prec() + 64);
	f->SetUint64(5);

	while (p_n > 0) {
		if ((p_n & 1) != 0) {
			Mul(this, f);
		}
		f->Mul(f, f);
		p_n >>= 1;
	}
}

// SetString sets z to the value of s and returns z and a boolean indicating
// success. s must be a floating-point number of the same format as accepted
// by [Float.Parse], with base argument 0. The entire string (not just a prefix) must
// be valid for success. If the operation failed, the value of z is undefined
// but the returned value is nil.
//
// Parse parses s which must contain a text representation of a floating-
// point number with a mantissa in the given conversion base (the exponent
// is always a decimal number), or a string representing an infinite value.
//
// For base 0, an underscore character “_” may appear between a base
// prefix and an adjacent digit, and between successive digits; such
// underscores do not change the value of the number, or the returned
// digit count. Incorrect placement of underscores is reported as an
// error if there are no other errors. If base != 0, underscores are
// not recognized and thus terminate scanning like any other character
// that is not a valid radix point or digit.
//
// It sets z to the (possibly rounded) value of the corresponding floating-
// point value, and returns z, the actual base b, and an error err, if any.
// The entire string (not just a prefix) must be consumed for success.
// If z's precision is 0, it is changed to 64 before rounding takes effect.
// The number must be of the form:
//
//	number    = [ sign ] ( float | "inf" | "Inf" ) .
//	sign      = "+" | "-" .
//	float     = ( mantissa | prefix pmantissa ) [ exponent ] .
//	prefix    = "0" [ "b" | "B" | "o" | "O" | "x" | "X" ] .
//	mantissa  = digits "." [ digits ] | digits | "." digits .
//	pmantissa = [ "_" ] digits "." [ digits ] | [ "_" ] digits | "." digits .
//	exponent  = ( "e" | "E" | "p" | "P" ) [ sign ] digits .
//	digits    = digit { [ "_" ] digit } .
//	digit     = "0" ... "9" | "a" ... "z" | "A" ... "Z" .
//
// The base argument must be 0, 2, 8, 10, or 16. Providing an invalid base
// argument will lead to a run-time panic.
//
// For base 0, the number prefix determines the actual base: A prefix of
// “0b” or “0B” selects base 2, “0o” or “0O” selects base 8, and
// “0x” or “0X” selects base 16. Otherwise, the actual base is 10 and
// no prefix is accepted. The octal prefix "0" is not supported (a leading
// "0" is simply considered a "0").
//
// A "p" or "P" exponent indicates a base 2 (rather than base 10) exponent;
// for instance, "0x1.fffffffffffffp1023" (using base 0) represents the
// maximum float64 value. For hexadecimal mantissae, the exponent character
// must be one of 'p' or 'P', if present (an "e" or "E" exponent indicator
// cannot be distinguished from a mantissa digit).
//
// The returned *Float f is nil and the value of z is valid but not
// defined if an error is reported.
Error BigFloat::SetString(const godot::String &p_s, int64_t p_base) {
	// scan doesn't handle ±Inf
	if (p_s.length() == 3 && (p_s == "Inf" || p_s == "inf")) {
		SetInf(false);
		return OK;
	}
	if (p_s.length() == 4 && (p_s[0] == '+' || p_s[0] == '-') && (p_s[1] == 'I' || p_s[1] == 'i') && p_s[2] == 'n' && p_s[3] == 'f') {
		SetInf(p_s[0] == '-');
		return OK;
	}

	int64_t off = 0;
	const Error err = _scan(p_s, off, p_base);
	if (err != OK) {
		return err;
	}

	// entire string must have been consumed
	ERR_FAIL_COND_V(off != p_s.length(), ERR_INVALID_PARAMETER);

	return OK;
}
