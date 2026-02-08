// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"
#include "godot_big_naturals.h"

// This file implements string-to-Float conversion functions.

// scan is like Parse but reads the longest possible prefix representing a valid
// floating point number from an io.ByteScanner rather than a string. It serves
// as the implementation of Parse. It does not recognize ±Inf and does not expect
// EOF at the end.
Ref<BigFloat> BigFloat::scan(const PackedByteArray &buf, int64_t &i, int64_t &base) {
	const int64_t orig_base = base;

	uint32_t prec = _prec;
	if (prec == 0) {
		prec = 64;
	}

	// A reasonable value in case of an error.
	_form = FORM_ZERO;

	// sign
	ERR_FAIL_COND_V(!nat_scanSign(buf, i, _neg), nullptr);

	// mantissa
	int64_t fcount = 0; // fractional digit count; valid if <= 0
	ERR_FAIL_COND_V(!nat_scan(_mant, buf, i, base, fcount, true), nullptr);

	// exponent
	int64_t exp = 0, ebase = 0;
	ERR_FAIL_COND_V(!nat_scanExponent(buf, i, true, orig_base == 0, exp, ebase), nullptr);

	// special-case 0
	if (_mant.is_empty()) {
		_prec = prec;
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		return this;
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
	int64_t exp2 = int64_t(_mant.size()) * 64 - nat_fnorm(_mant);
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
			ERR_FAIL_V_MSG(nullptr, "unexpected mantissa base");
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
		ERR_FAIL_V_MSG(nullptr, "unexpected exponent base");
	}
	// exp consumed - not needed anymore

	ERR_FAIL_COND_V_MSG(MIN_EXP > exp2 || exp2 > MAX_EXP, nullptr, "exponent overflow");

	// apply 2**exp2
	_prec = prec;
	_form = FORM_FINITE;
	_exp = int32_t(exp2);

	if (exp5 == 0) {
		// no decimal exponent contribution
		round(0);
		return this;
	}

	// exp5 != 0

	// apply 5**exp5
	Ref<BigFloat> p;
	p.instantiate();
	p->SetPrec(Prec() + 64); // use more bits for p -- TODO(gri) what is the right number?
	if (exp5 < 0) {
		Quo(this, p->pow5(uint64_t(-exp5)));
	} else {
		Mul(this, p->pow5(uint64_t(exp5)));
	}

	return this;
}

// These powers of 5 fit into a uint64.
//
//	for p, q := uint64(0), uint64(1); p < q; p, q = q, q*5 {
//		fmt.Println(q)
//	}
static constexpr uint64_t pow5tab[] = {
	1LLU,
	5LLU,
	25LLU,
	125LLU,
	625LLU,
	3125LLU,
	15625LLU,
	78125LLU,
	390625LLU,
	1953125LLU,
	9765625LLU,
	48828125LLU,
	244140625LLU,
	1220703125LLU,
	6103515625LLU,
	30517578125LLU,
	152587890625LLU,
	762939453125LLU,
	3814697265625LLU,
	19073486328125LLU,
	95367431640625LLU,
	476837158203125LLU,
	2384185791015625LLU,
	11920928955078125LLU,
	59604644775390625LLU,
	298023223876953125LLU,
	1490116119384765625LLU,
	7450580596923828125LLU,
};

// pow5 sets z to 5**n and returns z.
// n must not be negative.
Ref<BigFloat> BigFloat::pow5(uint64_t n) {
	static constexpr uint64_t m = (sizeof(pow5tab) / sizeof(pow5tab[0])) - 1;
	if (n <= m) {
		return SetUint64(pow5tab[n]);
	}

	// n > m

	SetUint64(pow5tab[m]);
	n -= m;

	// use more bits for f than for z
	// TODO(gri) what is the right number?
	Ref<BigFloat> f;
	f.instantiate();
	f->SetPrec(Prec() + 64);
	f->SetUint64(5);

	while (n > 0) {
		if ((n & 1) != 0) {
			Mul(this, f);
		}
		f->Mul(f, f);
		n >>= 1;
	}

	return this;
}

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
Ref<BigFloat> BigFloat::SetString(const godot::String &s, int64_t base) {
	// scan doesn't handle ±Inf
	if (s.length() == 3 && (s == "Inf" || s == "inf")) {
		return SetInf(false);
	}

	if (s.length() == 4 && (s[0] == '+' || s[0] == '-') && (s.substr(1) == "Inf" || s.substr(1) == "inf")) {
		return SetInf(s[0] == '-');
	}

	PackedByteArray buf = s.to_ascii_buffer();

	int64_t i = 0;
	ERR_FAIL_COND_V(scan(buf, i, base).is_null(), nullptr);
	ERR_FAIL_COND_V_MSG(i != buf.size(), nullptr, "expected end of string");

	return this;
}
