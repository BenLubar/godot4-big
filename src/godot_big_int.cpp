// This file is ported from src/math/big/int.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements signed multi-precision integers.

#include "godot_big_int.h"

#include "godot_big_float.h"

#include <algorithm>
#include <bit>

using namespace godot;

// An Int represents a signed multi-precision integer.
// The zero value for an Int represents the value 0.
//
// Operations always take pointer arguments (*Int) rather
// than Int values, and each unique Int value requires
// its own unique *Int pointer. To "copy" an Int value,
// an existing (or newly allocated) Int must be set to
// a new value using the [Int.Set] method; shallow copies
// of Ints are not supported and may lead to errors.

extern Ref<BigInt> *intOne;

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x == 0;
//   - +1 if x > 0.
int BigInt::Sign() const {
	if (_abs.array.is_empty()) {
		return 0;
	}
	if (_neg) {
		return -1;
	}
	return 1;
}

// SetInt64 sets z to x and returns z.
void BigInt::SetInt64(int64_t p_x) {
	bool neg = false;
	if (p_x < 0) {
		neg = true;
		p_x = -p_x;
	}

	_abs.setUint64(static_cast<uint64_t>(p_x));
	_neg = neg;
	emit_changed();
}

// SetUint64 sets z to x and returns z.
void BigInt::SetUint64(uint64_t p_x) {
	_abs.setUint64(p_x);
	_neg = false;
	emit_changed();
}

// NewInt allocates and returns a new [Int] set to x.
Ref<BigInt> BigInt::NewInt(int64_t p_x) {
	Ref<BigInt> n{ memnew(BigInt) };
	n->SetInt64(p_x);

	return n;
}

// Set sets z to x and returns z.
void BigInt::Set(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_abs.set(p_x->_abs);
	_neg = p_x->_neg;
	emit_changed();
}

// Abs sets z to |x| (the absolute value of x) and returns z.
void BigInt::Abs(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_abs.set(p_x->_abs);
	_neg = false;
	emit_changed();
}

// Neg sets z to -x and returns z.
void BigInt::Neg(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_abs.set(p_x->_abs);
	_neg = !_abs.array.is_empty() && !p_x->_neg;
	emit_changed();
}

// Add sets z to the sum x+y and returns z.
void BigInt::Add(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	bool neg = p_x->_neg;
	if (p_x->_neg == p_y->_neg) {
		// x + y == x + y
		// (-x) + (-y) == -(x + y)
		_abs.add(p_x->_abs, p_y->_abs);
	} else {
		// x + (-y) == x - y == -(y - x)
		// (-x) + y == y - x == -(x - y)
		if (p_x->_abs.cmp(p_y->_abs) >= 0) {
			_abs.sub(p_x->_abs, p_y->_abs);
		} else {
			neg = !neg;
			_abs.sub(p_y->_abs, p_x->_abs);
		}
	}
	_neg = !_abs.array.is_empty() && neg; // 0 has no sign
	emit_changed();
}

// Sub sets z to the difference x-y and returns z.
void BigInt::Sub(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	bool neg = p_x->_neg;
	if (p_x->_neg != p_y->_neg) {
		// x - (-y) == x + y
		// (-x) - y == -(x + y)
		_abs.add(p_x->_abs, p_y->_abs);
	} else {
		// x - y == x - y == -(y - x)
		// (-x) - (-y) == y - x == -(x - y)
		if (p_x->_abs.cmp(p_y->_abs) >= 0) {
			_abs.sub(p_x->_abs, p_y->_abs);
		} else {
			neg = !neg;
			_abs.sub(p_y->_abs, p_x->_abs);
		}
	}
	_neg = !_abs.array.is_empty() && neg; // 0 has no sign
	emit_changed();
}

// Mul sets z to the product x*y and returns z.
void BigInt::Mul(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	// x * y == x * y
	// x * (-y) == -(x * y)
	// (-x) * y == -(x * y)
	// (-x) * (-y) == x * y
	if (p_x == p_y) {
		_abs.sqr(p_x->_abs);
		_neg = false;
	} else {
		_abs.mul(p_x->_abs, p_y->_abs);
		_neg = !_abs.array.is_empty() && p_x->_neg != p_y->_neg; // 0 has no sign
	}
	emit_changed();
}

// MulRange sets z to the product of all integers
// in the range [a, b] inclusively and returns z.
// If a > b (empty range), the result is 1.
void BigInt::MulRange(int64_t p_a, int64_t p_b) {
	if (p_a > p_b) {
		// empty range
		SetUint64(1);
		return;
	}

	if (p_a <= 0 && p_b >= 0) {
		// range includes 0
		SetUint64(0);
		return;
	}

	// a <= b && (b < 0 || a > 0)

	bool neg = false;
	if (p_a < 0) {
		neg = ((p_b - p_a) & 1) == 0;
		std::swap(p_a, p_b);
		p_a = -p_a;
		p_b = -p_b;
	}

	_abs.mulRange(static_cast<uint64_t>(p_a), static_cast<uint64_t>(p_b));
	_neg = neg;
	emit_changed();
}

// Binomial sets z to the binomial coefficient C(n, k) and returns z.
void BigInt::Binomial(int64_t p_n, int64_t p_k) {
	if (p_k > p_n) {
		SetUint64(0);
		return;
	}

	// reduce the number of multiplications by reducing k
	p_k = std::min(p_k, p_n - p_k); // C(n, k) == C(n, n-k)

	// C(n, k) == n * (n-1) * ... * (n-k+1) / k * (k-1) * ... * 1
	//         == n * (n-1) * ... * (n-k+1) / 1 * (1+1) * ... * k
	//
	// Using the multiplicative formula produces smaller values
	// at each step, requiring fewer allocations and computations:
	//
	// z = 1
	// for i := 0; i < k; i = i+1 {
	//     z *= n-i
	//     z /= i+1
	// }
	//
	// finally to avoid computing i+1 twice per loop:
	//
	// z = 1
	// i := 0
	// for i < k {
	//     z *= n-i
	//     i++
	//     z /= i
	// }
	Ref<BigInt> N{ memnew(BigInt) };
	Ref<BigInt> K{ memnew(BigInt) };
	Ref<BigInt> i{ memnew(BigInt) };
	Ref<BigInt> t{ memnew(BigInt) };
	Ref<BigInt> one{ memnew(BigInt) };

	N->SetInt64(p_n);
	K->SetInt64(p_k);
	one->SetUint64(1);

	Set(one);

	while (i->Cmp(K) < 0) {
		t->Sub(N, i);
		Mul(this, t);
		i->Add(i, one);
		Quo(this, i);
	}
}

// Quo sets z to the quotient x/y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Quo implements truncated division (like Go); see [Int.QuoRem] for more details.
Error BigInt::Quo(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	[[maybe_unused]] BigNat rem;
	_abs.div(rem, p_x->_abs, p_y->_abs);
	_neg = !_abs.array.is_empty() && p_x->_neg != p_y->_neg; // 0 has no sign

	emit_changed();
	return OK;
}

// Rem sets z to the remainder x%y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Rem implements truncated modulus (like Go); see [Int.QuoRem] for more details.
Error BigInt::Rem(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	[[maybe_unused]] BigNat quo;
	quo.div(_abs, p_x->_abs, p_y->_abs);
	_neg = !_abs.array.is_empty() && p_x->_neg; // 0 has no sign

	emit_changed();
	return OK;
}

// QuoRem sets z to the quotient x/y and r to the remainder x%y
// and returns the pair (z, r) for y != 0.
// If y == 0, a division-by-zero run-time panic occurs.
//
// QuoRem implements T-division and modulus (like Go):
//
//	q = x/y      with the result truncated to zero
//	r = x - y*q
//
// (See Daan Leijen, “Division and Modulus for Computer Scientists”.)
// See [Int.DivMod] for Euclidean division and modulus (unlike Go).
Error BigInt::QuoRem(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y, const Ref<BigInt> &r_r) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");
	ERR_FAIL_NULL_V(*r_r, ERR_INVALID_PARAMETER);

	_abs.div(r_r->_abs, p_x->_abs, p_y->_abs);
	const bool x_neg = p_x->_neg;
	_neg = !_abs.array.is_empty() && x_neg != p_y->_neg; // 0 has no sign
	r_r->_neg = !r_r->_abs.array.is_empty() && x_neg; // 0 has no sign

	emit_changed();
	r_r->emit_changed();
	return OK;
}

// Div sets z to the quotient x/y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Div implements Euclidean division (unlike Go); see [Int.DivMod] for more details.
Error BigInt::Div(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	const bool y_neg = p_y->_neg; // z may be an alias for y

	Ref<BigInt> r{ memnew(BigInt) };

	QuoRem(p_x, p_y, r);

	if (r->_neg) {
		if (y_neg) {
			Add(this, *intOne);
		} else {
			Sub(this, *intOne);
		}
	}

	return OK;
}

// Mod sets z to the modulus x%y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Mod implements Euclidean modulus (unlike Go); see [Int.DivMod] for more details.
Error BigInt::Mod(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	Ref<BigInt> y = p_y; // save y
	if (this == *p_y) {
		y.instantiate();
		y->Set(p_y);
	}

	Ref<BigInt> q{ memnew(BigInt) };
	q->QuoRem(p_x, p_y, this);

	if (_neg) {
		if (y->_neg) {
			Sub(this, y);
		} else {
			Add(this, y);
		}
	}

	return OK;
}

// DivMod sets z to the quotient x div y and m to the modulus x mod y
// and returns the pair (z, m) for y != 0.
// If y == 0, a division-by-zero run-time panic occurs.
//
// DivMod implements Euclidean division and modulus (unlike Go):
//
//	q = x div y  such that
//	m = x - y*q  with 0 <= m < |y|
//
// (See Raymond T. Boute, “The Euclidean definition of the functions
// div and mod”. ACM Transactions on Programming Languages and
// Systems (TOPLAS), 14(2):127-144, New York, NY, USA, 4/1992.
// ACM press.)
// See [Int.QuoRem] for T-division and modulus (like Go).
Error BigInt::DivMod(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y, const Ref<BigInt> &r_m) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");
	ERR_FAIL_NULL_V(*r_m, ERR_INVALID_PARAMETER);

	Ref<BigInt> y = p_y; // save y
	if (this == *p_y) {
		y.instantiate();
		y->Set(p_y);
	}

	QuoRem(p_x, p_y, r_m);

	if (r_m->_neg) {
		if (y->_neg) {
			Add(this, *intOne);
			r_m->Sub(r_m, y);
		} else {
			Sub(this, *intOne);
			r_m->Add(r_m, y);
		}
	}

	return OK;
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y;
//   - +1 if x > y.
int BigInt::Cmp(const Ref<BigInt> &p_y) const {
	ERR_FAIL_NULL_V(*p_y, 0);

	// x cmp y == x cmp y
	// x cmp (-y) == x
	// (-x) cmp y == y
	// (-x) cmp (-y) == -(x cmp y)
	if (this == *p_y) {
		return 0;
	}
	if (_neg == p_y->_neg) {
		const int r = _abs.cmp(p_y->_abs);
		return _neg ? -r : r;
	}
	return _neg ? -1 : 1;
}

// CmpAbs compares the absolute values of x and y and returns:
//   - -1 if |x| < |y|;
//   - 0 if |x| == |y|;
//   - +1 if |x| > |y|.
int BigInt::CmpAbs(const Ref<BigInt> &p_y) const {
	ERR_FAIL_NULL_V(*p_y, 0);

	return _abs.cmp(p_y->_abs);
}

// low32 returns the least significant 32 bits of x.
uint32_t BigNat::low32() const {
	if (array.is_empty()) {
		return 0;
	}

	return static_cast<uint32_t>(array[0]);
}

// low64 returns the least significant 64 bits of x.
uint64_t BigNat::low64() const {
	if (array.is_empty()) {
		return 0;
	}

	return static_cast<uint64_t>(array[0]);
}

// Int64 returns the int64 representation of x.
// If x cannot be represented in an int64, the result is undefined.
int64_t BigInt::Int64() const {
	int64_t v = static_cast<int64_t>(_abs.low64());
	if (_neg) {
		v = -v;
	}
	return v;
}

// Uint64 returns the uint64 representation of x.
// If x cannot be represented in a uint64, the result is undefined.
uint64_t BigInt::Uint64() const {
	return _abs.low64();
}

// IsInt64 reports whether x can be represented as an int64.
bool BigInt::IsInt64() const {
	if (_abs.array.size() <= 1) {
		const int64_t w = static_cast<int64_t>(_abs.low64());
		return w >= 0 || (_neg && w == -w);
	}
	return false;
}

// IsUint64 reports whether x can be represented as a uint64.
bool BigInt::IsUint64() const {
	return !_neg && _abs.array.size() <= 1;
}

// Float64 returns the float64 value nearest x,
// and an indication of any rounding that occurred.
Pair<double, BigAccuracy> BigInt::Float64() const {
	const int64_t n = _abs.bitLen();
	if (n == 0) {
		return { 0.0, ACC_EXACT };
	}

	// Fast path: no more than 53 significant bits.
	if (n <= 53 || (n < 64 && n - static_cast<int64_t>(_abs.trailingZeroBits()) <= 53)) {
		double f = static_cast<double>(_abs.low64());
		if (_neg) {
			f = -f;
		}
		return { f, ACC_EXACT };
	}

	Ref<BigFloat> f{ memnew(BigFloat) };
	f->SetInt(const_cast<BigInt *>(this));
	return f->Float64();
}

// SetString sets z to the value of s, interpreted in the given base,
// and returns z and a boolean indicating success. The entire string
// (not just a prefix) must be valid for success. If SetString fails,
// the value of z is undefined but the returned value is nil.
//
// The base argument must be 0 or a value between 2 and [MaxBase].
// For base 0, the number prefix determines the actual base: A prefix of
// “0b” or “0B” selects base 2, “0”, “0o” or “0O” selects base 8,
// and “0x” or “0X” selects base 16. Otherwise, the selected base is 10
// and no prefix is accepted.
//
// For bases <= 36, lower and upper case letters are considered the same:
// The letters 'a' to 'z' and 'A' to 'Z' represent digit values 10 to 35.
// For bases > 36, the upper case letters 'A' to 'Z' represent the digit
// values 36 to 61.
//
// For base 0, an underscore character “_” may appear between a base
// prefix and an adjacent digit, and between successive digits; such
// underscores do not change the value of the number.
// Incorrect placement of underscores is reported as an error if there
// are no other errors. If base != 0, underscores are not recognized
// and act like any other character that is not a valid digit.
Error BigInt::SetString(const godot::String &p_s, int64_t p_base) {
	int64_t off = 0;
	const Error err = _scan(p_s, off, p_base);
	if (err != OK) {
		return err;
	}

	ERR_FAIL_COND_V(p_s.length() != off, ERR_INVALID_PARAMETER);

	return OK;
}

// SetBytes interprets buf as the bytes of a big-endian unsigned
// integer, sets z to that value, and returns z.
void BigInt::SetBytes(const PackedByteArray &p_buf) {
	_abs.setBytes(p_buf);
	_neg = false;
	emit_changed();
}

// Bytes returns the absolute value of x as a big-endian byte slice.
//
// To use a fixed length slice, or a preallocated one, use [Int.FillBytes].
PackedByteArray BigInt::Bytes() const {
	PackedByteArray buf;
	buf.resize(_abs.array.size() * 8);
	return buf.slice(_abs.bytes(buf));
}

// FillBytes sets buf to the absolute value of x, storing it as a zero-extended
// big-endian byte slice, and returns buf.
//
// If the absolute value of x doesn't fit in buf, FillBytes will panic.
void BigInt::FillBytes(PackedByteArray &r_buf) const {
	// Clear whole buffer.
	r_buf.fill(0);

	(void)_abs.bytes(r_buf);
}

// BitLen returns the length of the absolute value of x in bits.
// The bit length of 0 is 0.
int64_t BigInt::BitLen() const {
	return _abs.bitLen();
}

// TrailingZeroBits returns the number of consecutive least significant zero
// bits of |x|.
uint64_t BigInt::TrailingZeroBits() const {
	return _abs.trailingZeroBits();
}

// Exp sets z = x**y mod |m| (i.e. the sign of m is ignored), and returns z.
// If m == nil or m == 0, z = x**y unless y <= 0 then z = 1. If m != 0, y < 0,
// and x and m are not relatively prime, z is unchanged and nil is returned.
Error BigInt::Exp(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y, const Ref<BigInt> &p_m) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);

	return _exp(p_x, p_y, p_m, false);
}

Error BigInt::expSlow(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y, const Ref<BigInt> &p_m) {
	return _exp(p_x, p_y, p_m, true);
}

Error BigInt::_exp(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y, const Ref<BigInt> &p_m, bool p_slow) {
	// See Knuth, volume 2, section 4.6.3.
	BigNat xWords = p_x->_abs;
	if (p_y->_neg) {
		if (p_m.is_null() || p_m->_abs.array.is_empty()) {
			SetUint64(1);
			return OK;
		}

		// for y < 0: x**y mod m == (x**(-1))**|y| mod m
		Ref<BigInt> inverse{ memnew(BigInt) };
		const Error err = inverse->ModInverse(p_x, p_m);
		if (err != OK) {
			return err;
		}
		xWords = inverse->_abs;
	}
	BigNat yWords = p_y->_abs;

	BigNat mWords;
	if (p_m.is_valid()) {
		mWords = p_m->_abs; // m.abs may be nil for m == 0
	}

	_abs.expNN(xWords, yWords, mWords, p_slow);
	_neg = !_abs.array.is_empty() && p_x->_neg && !yWords.array.is_empty() && (yWords[0] & 1) == 1; // 0 has no sign
	if (_neg && !mWords.array.is_empty()) {
		// make modulus result positive
		_abs.sub(mWords, _abs); // z == x**y mod |m| && 0 <= z < |m|
		_neg = false;
	}

	return OK;
}

// GCD sets z to the greatest common divisor of a and b and returns z.
// If x or y are not nil, GCD sets their value such that z = a*x + b*y.
//
// a and b may be positive, zero or negative. (Before Go 1.14 both had
// to be > 0.) Regardless of the signs of a and b, z is always >= 0.
//
// If a == b == 0, GCD sets z = x = y = 0.
//
// If a == 0 and b != 0, GCD sets z = |b|, x = 0, y = sign(b) * 1.
//
// If a != 0 and b == 0, GCD sets z = |a|, x = sign(a) * 1, y = 0.
void BigInt::GCD(const Ref<BigInt> &r_x, const Ref<BigInt> &r_y, const Ref<BigInt> &p_a, const Ref<BigInt> &p_b) {
	ERR_FAIL_NULL(*p_a);
	ERR_FAIL_NULL(*p_b);

	if (p_a->_abs.array.is_empty() || p_b->_abs.array.is_empty()) {
		const int64_t lenA = p_a->_abs.array.size();
		const int64_t lenB = p_b->_abs.array.size();
		const bool negA = p_a->_neg;
		const bool negB = p_b->_neg;

		if (lenA == 0) {
			Abs(p_b);
		} else {
			Abs(p_a);
		}

		if (r_x.is_valid()) {
			if (lenA == 0) {
				r_x->SetUint64(0);
			} else {
				r_x->SetInt64(negA ? -1 : 1);
			}
		}

		if (r_y.is_valid()) {
			if (lenB == 0) {
				r_y->SetUint64(0);
			} else {
				r_y->SetInt64(negB ? -1 : 1);
			}
		}

		return;
	}

	_lehmerGCD(r_x, r_y, p_a, p_b);
	emit_changed();
}

// lehmerSimulate attempts to simulate several Euclidean update steps
// using the leading digits of A and B.  It returns u0, u1, v0, v1
// such that A and B can be updated as:
//
//	A = u0*A + v0*B
//	B = u1*A + v1*B
//
// Requirements: A >= B and len(B.abs) >= 2
// Since we are calculating with full words to avoid overflow,
// we use 'even' to track the sign of the cosequences.
// For even iterations: u0, v1 >= 0 && u1, v0 <= 0
// For odd  iterations: u0, v1 <= 0 && u1, v0 >= 0
void BigInt::_lehmerSimulate(const Ref<BigInt> &A, const Ref<BigInt> &B, BigWord &u0, BigWord &u1, BigWord &v0, BigWord &v1, bool &even) {
	// initialize the digits
	BigWord a1 = 0;
	BigWord a2 = 0;
	BigWord u2 = 0;
	BigWord v2 = 0;

	const int64_t m = B->_abs.array.size(); // m >= 2
	const int64_t n = A->_abs.array.size(); // n >= m >= 2

	// extract the top Word of bits from A and B
	const uint64_t h = std::countl_zero(A->_abs[n - 1]);
	a1 = (A->_abs[n - 1] << h) | (A->_abs[n - 2] >> (64 - h));
	// B may have implicit zero words in the high bits if the lengths differ
	if (n == m) {
		a2 = (B->_abs[n - 1] << h) | (B->_abs[n - 2] >> (64 - h));
	} else if (n == m + 1) {
		a2 = B->_abs[n - 2] >> (64 - h);
	} else {
		a2 = 0;
	}

	// Since we are calculating with full words to avoid overflow,
	// we use 'even' to track the sign of the cosequences.
	// For even iterations: u0, v1 >= 0 && u1, v0 <= 0
	// For odd  iterations: u0, v1 <= 0 && u1, v0 >= 0
	// The first iteration starts with k=1 (odd).
	even = false;
	// variables to track the cosequences
	u0 = 0;
	u1 = 1;
	u2 = 0;
	v0 = 0;
	v1 = 0;
	v2 = 1;

	// Calculate the quotient and cosequences using Collins' stopping condition.
	// Note that overflow of a Word is not possible when computing the remainder
	// sequence and cosequences since the cosequence size is bounded by the input size.
	// See section 4.2 of Jebelean for details.
	while (a2 >= v2 && a1 - a2 >= v1 + v2) {
		const BigWord q = a1 / a2;
		const BigWord r = a1 % a2;

		a1 = a2;
		a2 = r;

		std::tie(u0, u1, u2) = std::make_tuple(u1, u2, u1 + (q * u2));
		std::tie(v0, v1, v2) = std::make_tuple(v1, v2, v1 + (q * v2));
		even = !even;
	}
}

// lehmerUpdate updates the inputs A and B such that:
//
//	A = u0*A + v0*B
//	B = u1*A + v1*B
//
// where the signs of u0, u1, v0, v1 are given by even
// For even == true: u0, v1 >= 0 && u1, v0 <= 0
// For even == false: u0, v1 <= 0 && u1, v0 >= 0
// q, r, s, t are temporary variables to avoid allocations in the multiplication.
void BigInt::_lehmerUpdate(const Ref<BigInt> &A, const Ref<BigInt> &B, const Ref<BigInt> &q, const Ref<BigInt> &r, BigWord u0, BigWord u1, BigWord v0, BigWord v1, bool even) {
	q->_mulW(B, even, v0);
	r->_mulW(A, even, u1);
	A->_mulW(A, !even, u0);
	B->_mulW(B, !even, v1);
	A->Add(A, q);
	B->Add(B, r);
}

// mulW sets z = x * (-?)w
// where the minus sign is present when neg is true.
void BigInt::_mulW(const Ref<BigInt> &x, bool neg, BigWord w) {
	_abs.mulAddWW(x->_abs, w, 0);
	_neg = x->_neg != neg;
}

// euclidUpdate performs a single step of the Euclidean GCD algorithm
// if extended is true, it also updates the cosequence Ua, Ub.
// q and r are used as temporaries; the initial values are ignored.
void BigInt::_euclidUpdate(Ref<BigInt> &A, Ref<BigInt> &B, Ref<BigInt> &Ua, Ref<BigInt> &Ub, const Ref<BigInt> &q, Ref<BigInt> &r, bool extended) {
	q->QuoRem(A, B, r);

	if (extended) {
		// Ua, Ub = Ub, Ua-q*Ub
		q->Mul(q, Ub);
		std::swap(Ua, Ub);
		Ub->Sub(Ub, q);
	}

	const Ref<BigInt> tmp = A;
	A = B;
	B = r;
	r = tmp;
}

// lehmerGCD sets z to the greatest common divisor of a and b,
// which both must be != 0, and returns z.
// If x or y are not nil, their values are set such that z = a*x + b*y.
// See Knuth, The Art of Computer Programming, Vol. 2, Section 4.5.2, Algorithm L.
// This implementation uses the improved condition by Collins requiring only one
// quotient and avoiding the possibility of single Word overflow.
// See Jebelean, "Improving the multiprecision Euclidean algorithm",
// Design and Implementation of Symbolic Computation Systems, pp 45-58.
// The cosequences are updated according to Algorithm 10.45 from
// Cohen et al. "Handbook of Elliptic and Hyperelliptic Curve Cryptography" pp 192.
void BigInt::_lehmerGCD(const Ref<BigInt> &r_x, const Ref<BigInt> &r_y, const Ref<BigInt> &p_a, const Ref<BigInt> &p_b) {
	Ref<BigInt> A{ memnew(BigInt) };
	Ref<BigInt> B{ memnew(BigInt) };
	Ref<BigInt> Ua;
	Ref<BigInt> Ub;
	A->Abs(p_a);
	B->Abs(p_b);

	const bool extended = r_x.is_valid() || r_y.is_valid();

	if (extended) {
		// Ua (Ub) tracks how many times input a has been accumulated into A (B).
		Ua.instantiate();
		Ub.instantiate();
		Ua->SetUint64(1);
	}

	// temp variables for multiprecision update
	Ref<BigInt> q{ memnew(BigInt) };
	Ref<BigInt> r{ memnew(BigInt) };

	// ensure A >= B
	if (A->CmpAbs(B) < 0) {
		std::swap(A, B);
		std::swap(Ua, Ub);
	}

	// loop invariant A >= B
	while (B->_abs.array.size() > 1) {
		// Attempt to calculate in single-precision using leading words of A and B.
		BigWord u0;
		BigWord u1;
		BigWord v0;
		BigWord v1;
		bool even;
		_lehmerSimulate(A, B, u0, u1, v0, v1, even);

		// multiprecision Step
		if (v0 != 0) {
			// Simulate the effect of the single-precision steps using the cosequences.
			// A = u0*A + v0*B
			// B = u1*A + v1*B
			_lehmerUpdate(A, B, q, r, u0, u1, v0, v1, even);

			if (extended) {
				// Ua = u0*Ua + v0*Ub
				// Ub = u1*Ua + v1*Ub
				_lehmerUpdate(Ua, Ub, q, r, u0, u1, v0, v1, even);
			}
		} else {
			// Single-digit calculations failed to simulate any quotients.
			// Do a standard Euclidean step.
			_euclidUpdate(A, B, Ua, Ub, q, r, extended);
		}
	}

	if (!B->_abs.array.is_empty()) {
		// extended Euclidean algorithm base case if B is a single Word
		if (A->_abs.array.size() > 1) {
			// A is longer than a single Word, so one update is needed.
			_euclidUpdate(A, B, Ua, Ub, q, r, extended);
		}

		if (!B->_abs.array.is_empty()) {
			// A and B are both a single Word.
			BigWord aWord = A->_abs[0];
			BigWord bWord = B->_abs[0];
			if (extended) {
				BigWord ua = 1;
				BigWord ub = 0;
				BigWord va = 0;
				BigWord vb = 1;
				bool even = true;
				while (bWord != 0) {
					const BigWord q = aWord / bWord;
					const BigWord r = aWord % bWord;

					aWord = bWord;
					bWord = r;

					std::tie(ua, ub) = std::make_tuple(ub, ua + (q * ub));
					std::tie(va, vb) = std::make_tuple(vb, va + (q * vb));

					even = !even;
				}

				Ua->_mulW(Ua, !even, ua);
				Ub->_mulW(Ub, even, va);
				Ua->Add(Ua, Ub);
			} else {
				while (bWord != 0) {
					BigWord tmp = bWord;
					bWord = aWord % bWord;
					aWord = tmp;
				}
			}
			A->_abs[0] = aWord;
		}
	}

	const bool negA = p_a->_neg;
	if (r_y.is_valid()) {
		// avoid aliasing b needed in the division below
		if (r_y == p_b) {
			B->Set(p_b);
		} else {
			B = p_b;
		}

		// y = (z - a*x)/b
		r_y->Mul(p_a, Ua); // y can safely alias a
		if (negA) {
			r_y->_neg = !r_y->_neg;
		}
		r_y->Sub(A, r_y);
		r_y->Div(r_y, B);
	}

	if (r_x.is_valid()) {
		r_x->Set(Ua);
		if (negA) {
			r_x->_neg = !r_x->_neg;
		}
		r_x->emit_changed();
	}

	Set(A);
}

// Rand sets z to a pseudo-random number in [0, n) and returns z.
//
// As this uses the [math/rand] package, it must not be used for
// security-sensitive work. Use [crypto/rand.Int] instead.
void BigInt::Rand(const std::function<uint32_t()> &p_rnd, const Ref<BigInt> &p_n) {
	// z.neg is not modified before the if check, because z and n might alias.
	if (p_n.is_null() || p_n->_neg || p_n->_abs.array.is_empty()) {
		_neg = false;
		_abs.array.clear();
	} else {
		_neg = false;
		_abs.random(p_rnd, p_n->_abs, p_n->_abs.bitLen());
	}

	emit_changed();
}

void BigInt::Rand_bind(const Callable &p_rnd, const Ref<BigInt> &p_n) {
	Rand(
			[p_rnd]() -> uint32_t {
				return p_rnd.call();
			},
			p_n);
}

// ModInverse sets z to the multiplicative inverse of g in the ring ℤ/nℤ
// and returns z. If g and n are not relatively prime, g has no multiplicative
// inverse in the ring ℤ/nℤ.  In this case, z is unchanged and the return value
// is nil. If n == 0, a division-by-zero run-time panic occurs.
Error BigInt::ModInverse(const Ref<BigInt> &p_g, const Ref<BigInt> &p_n) {
	ERR_FAIL_NULL_V(*p_g, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_n, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_g->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	// GCD expects parameters a and b to be > 0.
	Ref<BigInt> n = p_n;
	if (n->_neg) {
		n.instantiate();
		n->Neg(p_n);
	}
	Ref<BigInt> g = p_g;
	if (g->_neg) {
		g.instantiate();
		g->Mod(p_g, n);
	}
	Ref<BigInt> d{ memnew(BigInt) };
	Ref<BigInt> x{ memnew(BigInt) };
	d->GCD(x, nullptr, g, n);

	// if and only if d==1, g and n are relatively prime
	if (d->Cmp(*intOne) != 0) {
		return ERR_PARAMETER_RANGE_ERROR;
	}

	// x and y are such that g*x + n*y = 1, therefore x is the inverse element,
	// but it may be negative, so convert to the range 0 <= z < |n|
	if (x->_neg) {
		Add(x, n);
	} else {
		Set(x);
	}

	return OK;
}

Error BigNat::modInverse(BigNat g, BigNat n) {
	// TODO(rsc): ModInverse should be implemented in terms of this function.
	Ref<BigInt> z{ memnew(BigInt) };
	Ref<BigInt> gi{ memnew(BigInt) };
	Ref<BigInt> ni{ memnew(BigInt) };

	z->_abs = *this;
	// NOLINTBEGIN(performance-unnecessary-value-param)
	gi->_abs = g;
	ni->_abs = n;
	// NOLINTEND(performance-unnecessary-value-param)

	const Error err = z->ModInverse(gi, ni);
	*this = z->_abs;

	return err;
}

// Jacobi returns the Jacobi symbol (x/y), either +1, -1, or 0.
// The y argument must be an odd integer.
int BigInt::Jacobi(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL_V(*p_x, 0);
	ERR_FAIL_NULL_V(*p_y, 0);
	ERR_FAIL_COND_V_MSG(p_y->_abs.array.is_empty() || (p_y->_abs[0] & 1) == 0, 0, "invalid 2nd argument to BigInt::Jacobi: need odd integer");

	// We use the formulation described in chapter 2, section 2.4,
	// "The Yacas Book of Algorithms":
	// http://yacas.sourceforge.net/Algo.book.pdf

	Ref<BigInt> a{ memnew(BigInt) };
	Ref<BigInt> b{ memnew(BigInt) };
	Ref<BigInt> c{ memnew(BigInt) };
	a->Set(p_x);
	b->Set(p_y);
	int j = 1;

	if (b->_neg) {
		if (a->_neg) {
			j = -1;
		}
		b->_neg = false;
	}

	while (true) {
		if (b->Cmp(*intOne) == 0) {
			return j;
		}
		if (a->_abs.array.is_empty()) {
			return 0;
		}
		a->Mod(a, b);
		if (a->_abs.array.is_empty()) {
			return 0;
		}
		// a > 0

		// handle factors of 2 in 'a'
		const uint64_t s = a->_abs.trailingZeroBits();
		if ((s & 1) != 0) {
			const uint64_t bmod8 = b->_abs[0] & 7;
			if (bmod8 == 3 || bmod8 == 5) {
				j = -j;
			}
		}
		c->Rsh(a, s); // a = 2^s*c

		// swap numerator and denominator
		if ((b->_abs[0] & 3) == 3 && (c->_abs[0] & 3) == 3) {
			j = -j;
		}
		a->Set(b);
		b->Set(c);
	}
}

// modSqrt3Mod4 uses the identity
//
//	   (a^((p+1)/4))^2  mod p
//	== u^(p+1)          mod p
//	== u^2              mod p
//
// to calculate the square root of any quadratic residue mod p quickly for 3
// mod 4 primes.
void BigInt::_modSqrt3Mod4Prime(const Ref<BigInt> &p_x, const Ref<BigInt> &p_p) {
	Ref<BigInt> e{ memnew(BigInt) };
	e->Add(p_p, *intOne); // e = p + 1
	e->Rsh(e, 2); // e = (p + 1) / 4
	Exp(p_x, e, p_p); // z = x^e mod p
}

// modSqrt5Mod8Prime uses Atkin's observation that 2 is not a square mod p
//
//	alpha ==  (2*a)^((p-5)/8)    mod p
//	beta  ==  2*a*alpha^2        mod p  is a square root of -1
//	b     ==  a*alpha*(beta-1)   mod p  is a square root of a
//
// to calculate the square root of any quadratic residue mod p quickly for 5
// mod 8 primes.
void BigInt::_modSqrt5Mod8Prime(const Ref<BigInt> &p_x, const Ref<BigInt> &p_p) {
	// p == 5 mod 8 implies p = e*8 + 5
	// e is the quotient and 5 the remainder on division by 8
	Ref<BigInt> e{ memnew(BigInt) };
	Ref<BigInt> tx{ memnew(BigInt) };
	Ref<BigInt> alpha{ memnew(BigInt) };
	Ref<BigInt> beta{ memnew(BigInt) };
	e->Rsh(p_p, 3); // e = (p - 5) / 8
	tx->Lsh(p_x, 1); // tx = 2*x
	alpha->Exp(tx, e, p_p);
	beta->Mul(alpha, alpha);
	beta->Mod(beta, p_p);
	beta->Mul(beta, tx);
	beta->Mod(beta, p_p);
	beta->Sub(beta, *intOne);
	beta->Mul(beta, p_x); // NOLINT(readability-suspicious-call-argument)
	beta->Mod(beta, p_p);
	beta->Mul(beta, alpha);
	Mod(beta, p_p);
}

// modSqrtTonelliShanks uses the Tonelli-Shanks algorithm to find the square
// root of a quadratic residue modulo any prime.
void BigInt::_modSqrtTonelliShanks(const Ref<BigInt> &p_x, const Ref<BigInt> &p_p) {
	// Break p-1 into s*2^e such that s is odd.
	Ref<BigInt> s{ memnew(BigInt) };
	s->Sub(p_p, *intOne);
	const uint64_t e = s->_abs.trailingZeroBits();
	s->Rsh(s, e);

	// find some non-square n
	const Ref<BigInt> n = BigInt::NewInt(2);
	while (Jacobi(n, p_p) != -1) {
		n->Add(n, *intOne);
	}

	// Core of the Tonelli-Shanks algorithm. Follows the description in
	// section 6 of "Square roots from 1; 24, 51, 10 to Dan Shanks" by Ezra
	// Brown:
	// https://www.maa.org/sites/default/files/pdf/upload_library/22/Polya/07468342.di020786.02p0470a.pdf
	Ref<BigInt> y{ memnew(BigInt) };
	Ref<BigInt> b{ memnew(BigInt) };
	Ref<BigInt> g{ memnew(BigInt) };
	Ref<BigInt> t{ memnew(BigInt) };

	y->Add(s, *intOne);
	y->Rsh(y, 1);
	y->Exp(p_x, y, p_p); // y = x^((s+1)/2)
	b->Exp(p_x, s, p_p); // b = x^s
	g->Exp(n, s, p_p); // g = n^s
	uint64_t r = e;
	while (true) {
		// find the least m such that ord_p(b) = 2^m
		uint64_t m = 0;
		t->Set(b);
		while (t->Cmp(*intOne) != 0) {
			t->Mul(t, t);
			t->Mod(t, p_p);
			m++;
		}

		if (m == 0) {
			Set(y);
			return;
		}

		t->SetUint64(0);
		t->SetBit(t, static_cast<int64_t>(r - m - 1), 1);
		t->Exp(g, t, p_p);
		// t = g^(2^(r-m-1)) mod p
		g->Mul(t, t);
		g->Mod(g, p_p); // g = g^(2^(r-m)) mod p
		y->Mul(y, t);
		y->Mod(y, p_p);
		b->Mul(b, g);
		b->Mod(b, p_p);
		r = m;
	}
}

// ModSqrt sets z to a square root of x mod p if such a square root exists, and
// returns z. The modulus p must be an odd prime. If x is not a square mod p,
// ModSqrt leaves z unchanged and returns nil. This function panics if p is
// not an odd integer, its behavior is undefined if p is odd but not prime.
Error BigInt::ModSqrt(const Ref<BigInt> &p_x, const Ref<BigInt> &p_p) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_p, ERR_INVALID_PARAMETER);

	switch (Jacobi(p_x, p_p)) {
		case -1:
			return ERR_PARAMETER_RANGE_ERROR; // x is not a square mod p
		case 0:
			SetUint64(0); // sqrt(0) mod p = 0
			return OK;
		case 1:
			break;
	}

	Ref<BigInt> x = p_x;
	if (x->_neg || x->Cmp(p_p) >= 0) { // ensure 0 <= x < p
		x.instantiate();
		x->Mod(p_x, p_p);
	}

	if (p_p->_abs[0] % 4 == 3) {
		// Check whether p is 3 mod 4, and if so, use the faster algorithm.
		_modSqrt3Mod4Prime(x, p_p);
		return OK;
	}

	if (p_p->_abs[0] % 8 == 5) {
		// Check whether p is 5 mod 8, use Atkin's algorithm.
		_modSqrt5Mod8Prime(x, p_p);
		return OK;
	}

	// Otherwise, use Tonelli-Shanks.
	_modSqrtTonelliShanks(x, p_p);
	return OK;
}

// Lsh sets z = x << n and returns z.
void BigInt::Lsh(const Ref<BigInt> &p_x, uint64_t p_n) {
	ERR_FAIL_NULL(*p_x);

	_abs.lsh(p_x->_abs, p_n);
	_neg = p_x->_neg;
	emit_changed();
}

// Rsh sets z = x >> n and returns z.
void BigInt::Rsh(const Ref<BigInt> &p_x, uint64_t p_n) {
	ERR_FAIL_NULL(*p_x);

	if (p_x->_neg) {
		// (-x) >> s == ^(x-1) >> s == ^((x-1) >> s) == -(((x-1) >> s) + 1)
		_abs.sub1(p_x->_abs); // no underflow because |x| > 0
		_abs.rsh(_abs, p_n);
		_abs.add1(_abs);
		_neg = true; // z cannot be zero if x is negative
		emit_changed();
		return;
	}

	_abs.rsh(p_x->_abs, p_n);
	_neg = false;
	emit_changed();
}

// Bit returns the value of the i'th bit of x. That is, it
// returns (x>>i)&1. The bit index i must be >= 0.
uint64_t BigInt::Bit(int64_t p_i) const {
	if (p_i == 0) {
		// optimization for common case: odd/even test of x
		if (_abs.array.is_empty()) {
			return 0;
		}
		return static_cast<uint64_t>(_abs[0] & 1); // bit 0 is same for -x
	}

	ERR_FAIL_COND_V_MSG(p_i < 0, 0, "negative bit index");

	if (_neg) {
		BigNat t;
		t.sub1(_abs);
		return t.bit(static_cast<uint64_t>(p_i)) ^ 1;
	}

	return _abs.bit(static_cast<uint64_t>(p_i));
}

// SetBit sets z to x, with x's i'th bit set to b (0 or 1).
// That is,
//   - if b is 1, SetBit sets z = x | (1 << i);
//   - if b is 0, SetBit sets z = x &^ (1 << i);
//   - if b is not 0 or 1, SetBit will panic.
void BigInt::SetBit(const Ref<BigInt> &p_x, int64_t p_i, uint64_t p_b) {
	ERR_FAIL_COND_MSG(p_i < 0, "negative bit index");

	if (p_x->_neg) {
		_abs.sub1(p_x->_abs);
		_abs.setBit(_abs, static_cast<uint64_t>(p_i), p_b ^ 1);
		_abs.add1(_abs);
		_neg = !_abs.array.is_empty();
		emit_changed();
		return;
	}

	_abs.setBit(p_x->_abs, static_cast<uint64_t>(p_i), p_b);
	_neg = false;
	emit_changed();
}

// And sets z = x & y and returns z.
void BigInt::And(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	if (p_x->_neg == p_y->_neg) {
		if (p_x->_neg) {
			// (-x) & (-y) == ^(x-1) & ^(y-1) == ^((x-1) | (y-1)) == -(((x-1) | (y-1)) + 1)
			BigNat x1;
			BigNat y1;
			x1.sub1(p_x->_abs);
			y1.sub1(p_y->_abs);
			_abs.or_(x1, y1);
			_abs.add1(_abs);
			_neg = true; // z cannot be zero if x and y are negative
			emit_changed();
			return;
		}

		// x & y == x & y
		_abs.and_(p_x->_abs, p_y->_abs);
		_neg = false;
		emit_changed();
		return;
	}

	// x.neg != y.neg
	Ref<BigInt> x = p_x;
	Ref<BigInt> y = p_y;
	if (x->_neg) {
		std::swap(x, y); // & is symmetric
	}

	// x & (-y) == x & ^(y-1) == x &^ (y-1)
	BigNat y1;
	y1.sub1(y->_abs);
	_abs.andNot(x->_abs, y1);
	_neg = false;
	emit_changed();
}

// AndNot sets z = x &^ y and returns z.
void BigInt::AndNot(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	if (p_x->_neg == p_y->_neg) {
		if (p_x->_neg) {
			// (-x) &^ (-y) == ^(x-1) &^ ^(y-1) == ^(x-1) & (y-1) == (y-1) &^ (x-1)
			BigNat x1;
			BigNat y1;
			x1.sub1(p_x->_abs);
			y1.sub1(p_y->_abs);
			_abs.andNot(y1, x1);
			_neg = false;
			emit_changed();
			return;
		}

		// x &^ y == x &^ y
		_abs.andNot(p_x->_abs, p_y->_abs);
		_neg = false;
		emit_changed();
		return;
	}

	if (p_x->_neg) {
		// (-x) &^ y == ^(x-1) &^ y == ^(x-1) & ^y == ^((x-1) | y) == -(((x-1) | y) + 1)
		BigNat x1;
		x1.sub1(p_x->_abs);
		_abs.or_(x1, p_y->_abs);
		_abs.add1(_abs);
		_neg = true; // z cannot be zero if x is negative and y is positive
		emit_changed();
		return;
	}

	// x &^ (-y) == x &^ ^(y-1) == x & (y-1)
	BigNat y1;
	y1.sub1(p_y->_abs);
	_abs.and_(p_x->_abs, y1);
	_neg = false;
	emit_changed();
}

// Or sets z = x | y and returns z.
void BigInt::Or(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	if (p_x->_neg == p_y->_neg) {
		if (p_x->_neg) {
			// (-x) | (-y) == ^(x-1) | ^(y-1) == ^((x-1) & (y-1)) == -(((x-1) & (y-1)) + 1)
			BigNat x1;
			BigNat y1;
			x1.sub1(p_x->_abs);
			y1.sub1(p_y->_abs);
			_abs.and_(x1, y1);
			_abs.add1(_abs);
			_neg = true; // z cannot be zero if x and y are negative
			emit_changed();
			return;
		}

		// x | y == x | y
		_abs.or_(p_x->_abs, p_y->_abs);
		_neg = false;
		emit_changed();
		return;
	}

	// x.neg != y.neg
	Ref<BigInt> x = p_x;
	Ref<BigInt> y = p_y;
	if (x->_neg) {
		std::swap(x, y); // | is symmetric
	}

	// x | (-y) == x | ^(y-1) == ^((y-1) &^ x) == -(^((y-1) &^ x) + 1)
	BigNat y1;
	y1.sub1(y->_abs);
	_abs.andNot(y1, p_x->_abs);
	_abs.add1(_abs);
	_neg = true; // z cannot be zero if one of x or y is negative
	emit_changed();
}

// Xor sets z = x ^ y and returns z.
void BigInt::Xor(const Ref<BigInt> &p_x, const Ref<BigInt> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	if (p_x->_neg == p_y->_neg) {
		if (p_x->_neg) {
			// (-x) ^ (-y) == ^(x-1) ^ ^(y-1) == (x-1) ^ (y-1)
			BigNat x1;
			BigNat y1;
			x1.sub1(p_x->_abs);
			y1.sub1(p_y->_abs);
			_abs.xor_(x1, y1);
			_neg = false;
			emit_changed();
			return;
		}

		// x ^ y == x ^ y
		_abs.xor_(p_x->_abs, p_y->_abs);
		_neg = false;
		emit_changed();
		return;
	}

	// x.neg != y.neg
	Ref<BigInt> x = p_x;
	Ref<BigInt> y = p_y;
	if (x->_neg) {
		std::swap(x, y); // ^ is symmetric
	}

	// x ^ (-y) == x ^ ^(y-1) == ^(x ^ (y-1)) == -((x ^ (y-1)) + 1)
	BigNat y1;
	y1.sub1(y->_abs);
	_abs.xor_(x->_abs, y1);
	_abs.add1(_abs);
	_neg = true; // z cannot be zero if only one of x or y is negative
	emit_changed();
}

// Not sets z = ^x and returns z.
void BigInt::Not(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	if (p_x->_neg) {
		// ^(-x) == ^(^(x-1)) == x-1
		_abs.sub1(p_x->_abs);
		_neg = false;
	} else {
		// ^x == -x-1 == -(x+1)
		_abs.add1(p_x->_abs);
		_neg = true; // z cannot be zero if x is positive
	}

	emit_changed();
}

// Sqrt sets z to ⌊√x⌋, the largest integer such that z² ≤ x, and returns z.
// It panics if x is negative.
Error BigInt::Sqrt(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_x->_neg, ERR_PARAMETER_RANGE_ERROR, "square root of negative number");

	_neg = false;
	_abs.sqrt(p_x->_abs);
	emit_changed();
	return OK;
}
