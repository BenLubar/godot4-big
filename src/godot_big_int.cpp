// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_int.h"
#include "godot_big_float.h"
#include "godot_big_naturals.h"

// This file implements signed multi-precision integers.

// An Int represents a signed multi-precision integer.
// The zero value for an Int represents the value 0.
//
// Operations always take pointer arguments (*Int) rather
// than Int values, and each unique Int value requires
// its own unique *Int pointer. To "copy" an Int value,
// an existing (or newly allocated) Int must be set to
// a new value using the [Int.Set] method; shallow copies
// of Ints are not supported and may lead to errors.
//
// Note that methods may leak the Int's value through timing side-channels.
// Because of this and because of the scope and complexity of the
// implementation, Int is not well-suited to implement cryptographic operations.
// The standard library avoids exposing non-trivial Int methods to
// attacker-controlled inputs and the determination of whether a bug in math/big
// is considered a security vulnerability might depend on the impact on the
// standard library.

/*
var intOne = &Int{false, natOne}
*/

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x == 0;
//   - +1 if x > 0.
int BigInt::Sign() const {
	if (_abs.is_empty()) {
		return 0;
	}

	if (_neg) {
		return -1;
	}

	return 1;
}

// SetInt64 sets z to x and returns z.
Ref<BigInt> BigInt::SetInt64(int64_t x) {
	bool neg = false;
	if (x < 0) {
		neg = true;
		x = -x;
	}

	nat_setUint64(_abs, uint64_t(x));
	_neg = neg;

	return this;
}

// SetUint64 sets z to x and returns z.
Ref<BigInt> BigInt::SetUint64(uint64_t x) {
	nat_setUint64(_abs, x);
	_neg = false;

	return this;
}

// NewInt allocates and returns a new [Int] set to x.
Ref<BigInt> BigInt::make(int64_t p_x) {
	Ref<BigInt> i;
	i.instantiate();

	if (p_x == 0) {
		return i;
	}

	const uint64_t u = p_x < 0 ? -p_x : p_x;
	i->_abs.resize(1);
	i->_abs[0] = u;
	i->_neg = p_x < 0;
	return i;
}

// Set sets z to x and returns z.
Ref<BigInt> BigInt::Set(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	if (this != *x) {
		nat_set(_abs, x->_abs);
		_neg = x->_neg;
	}

	return this;
}

// Abs sets z to |x| (the absolute value of x) and returns z.
Ref<BigInt> BigInt::Abs(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_neg = false;

	return this;
}

// Neg sets z to -x and returns z.
Ref<BigInt> BigInt::Neg(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_neg = !_abs.is_empty() && !_neg; // 0 has no sign

	return this;
}

// Add sets z to the sum x+y and returns z.
Ref<BigInt> BigInt::Add(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	bool neg = x->_neg;
	if (x->_neg == y->_neg) {
		// x + y == x + y
		// (-x) + (-y) == -(x + y)
		nat_add(_abs, x->_abs, y->_abs);
	} else {
		// x + (-y) == x - y == -(y - x)
		// (-x) + y == y - x == -(x - y)
		if (nat_cmp(x->_abs, y->_abs) >= 0) {
			nat_sub(_abs, x->_abs, y->_abs);
		} else {
			neg = !neg;
			nat_sub(_abs, y->_abs, x->_abs);
		}
	}

	_neg = !_abs.is_empty() && neg; // 0 has no sign

	return this;
}

// Sub sets z to the difference x-y and returns z.
Ref<BigInt> BigInt::Sub(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	bool neg = x->_neg;
	if (x->_neg != y->_neg) {
		// x - (-y) == x + y
		// (-x) - y == -(x + y)
		nat_add(_abs, x->_abs, y->_abs);
	} else {
		// x - y == x - y == -(y - x)
		// (-x) - (-y) == y - x == -(x - y)
		if (nat_cmp(x->_abs, y->_abs) >= 0) {
			nat_sub(_abs, x->_abs, y->_abs);
		} else {
			neg = !neg;
			nat_sub(_abs, y->_abs, x->_abs);
		}
	}

	_neg = !_abs.is_empty() && neg; // 0 has no sign

	return this;
}

// Mul sets z to the product x*y and returns z.
Ref<BigInt> BigInt::Mul(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x == y) {
		nat_sqr(_abs, x->_abs);
		_neg = false;

		return this;
	}

	nat_mul(_abs, x->_abs, y->_abs);
	_neg = !_abs.is_empty() && x->_neg != y->_neg; // 0 has no sign

	return this;
}

// MulRange sets z to the product of all integers
// in the range [a, b] inclusively and returns z.
// If a > b (empty range), the result is 1.
Ref<BigInt> BigInt::MulRange(int64_t a, int64_t b) {
	if (a > b) {
		return SetInt64(1); // empty range
	}

	if (a <= 0 && b >= 0) {
		return SetInt64(0); // range includes 0
	}

	// a <= b && (b < 0 || a > 0)

	bool neg = false;
	if (a < 0) {
		neg = ((b - a) & 1) == 0;
		std::swap(a, b);
		a = -a;
		b = -b;
	}

	nat_mulRange(_abs, uint64_t(a), uint64_t(b));
	_neg = neg;

	return this;
}

// Binomial sets z to the binomial coefficient C(n, k) and returns z.
Ref<BigInt> BigInt::Binomial(int64_t n, int64_t k) {
	if (k > n) {
		return SetInt64(0);
	}

	// reduce the number of multiplications by reducing k
	if (k > n - k) {
		k = n - k; // C(n, k) == C(n, n-k)
	}

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
	Ref<BigInt> N, K, i, t;
	N.instantiate();
	N->SetInt64(n);
	K.instantiate();
	K->SetInt64(k);
	i.instantiate();
	t.instantiate();
	Set(*intOne);

	while (i->Cmp(K) < 0) {
		Mul(this, t->Sub(N, i));
		i->Add(i, *intOne);
		Quo(this, i);
	}

	return this;
}

// Quo sets z to the quotient x/y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Quo implements truncated division (like Go); see [Int.QuoRem] for more details.
Ref<BigInt> BigInt::Quo(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	PackedInt64Array remainder;
	ERR_FAIL_COND_V_MSG(!nat_div(x->_abs, y->_abs, _abs, remainder), nullptr, "division by zero");
	_neg = !_abs.is_empty() && x->_neg != y->_neg; // 0 has no sign

	return this;
}

// Rem sets z to the remainder x%y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Rem implements truncated modulus (like Go); see [Int.QuoRem] for more details.
Ref<BigInt> BigInt::Rem(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	PackedInt64Array quotient;
	ERR_FAIL_COND_V_MSG(!nat_div(x->_abs, y->_abs, quotient, _abs), nullptr, "division by zero");
	_neg = !_abs.is_empty() && x->_neg; // 0 has no sign

	return this;
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
Ref<BigInt> BigInt::QuoRem(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &r) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);
	ERR_FAIL_NULL_V(*r, nullptr);

	const bool xneg = x->_neg;
	const bool yneg = y->_neg;

	nat_div(x->_abs, y->_abs, _abs, r->_abs);

	_neg = !_abs.is_empty() && xneg != yneg; // 0 has no sign
	r->_neg = !r->_abs.is_empty() && xneg;

	return this;
}

// Div sets z to the quotient x/y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Div implements Euclidean division (unlike Go); see [Int.DivMod] for more details.
Ref<BigInt> BigInt::Div(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	const bool y_neg = y->_neg; // z may be an alias for y

	Ref<BigInt> remainder;
	remainder.instantiate();

	ERR_FAIL_COND_V(QuoRem(x, y, remainder).is_null(), nullptr);
	if (remainder->_neg) {
		if (y_neg) {
			Add(this, *intOne);
		} else {
			Sub(this, *intOne);
		}
	}

	return this;
}

// Mod sets z to the modulus x%y for y != 0 and returns z.
// If y == 0, a division-by-zero run-time panic occurs.
// Mod implements Euclidean modulus (unlike Go); see [Int.DivMod] for more details.
Ref<BigInt> BigInt::Mod(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	Ref<BigInt> y0 = y; // save y
	if (this == *y) {
		y0.instantiate();
		y0->Set(y);
	}

	Ref<BigInt> q;
	q.instantiate();
	q->QuoRem(x, y, this);

	if (_neg) {
		if (y0->_neg) {
			Sub(this, y0);
		} else {
			Add(this, y0);
		}
	}

	return this;
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
Ref<BigInt> BigInt::DivMod(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &m) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);
	ERR_FAIL_NULL_V(*m, nullptr);

	Ref<BigInt> y0 = y; // save y
	if (this == *y) {
		y0.instantiate();
		y0->Set(y);
	}

	ERR_FAIL_COND_V(QuoRem(x, y, m).is_null(), nullptr);

	if (m->_neg) {
		if (y0->_neg) {
			Add(this, *intOne);
			m->Sub(m, y0);
		} else {
			Sub(this, *intOne);
			m->Add(m, y0);
		}
	}

	return this;
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y;
//   - +1 if x > y.
int BigInt::Cmp(const Ref<BigInt> &y) const {
	ERR_FAIL_NULL_V(*y, 0);

	// x cmp y == x cmp y
	// x cmp (-y) == x
	// (-x) cmp y == y
	// (-x) cmp (-y) == -(x cmp y)

	if (this == *y) {
		// nothing to do
		return 0;
	}

	if (_neg == y->_neg) {
		int r = nat_cmp(_abs, y->_abs);
		if (_neg) {
			r = -r;
		}

		return r;
	}

	if (_neg) {
		return -1;
	}

	return 1;
}

// CmpAbs compares the absolute values of x and y and returns:
//   - -1 if |x| < |y|;
//   - 0 if |x| == |y|;
//   - +1 if |x| > |y|.
int BigInt::CmpAbs(const Ref<BigInt> &y) const {
	ERR_FAIL_NULL_V(*y, 0);

	return nat_cmp(_abs, y->_abs);
}

// low32 returns the least significant 32 bits of x.
uint32_t nat_low32(PackedInt64Array x) {
	if (x.is_empty()) {
		return 0;
	}

	return uint32_t(x[0]);
}

// low64 returns the least significant 64 bits of x.
uint64_t nat_low64(PackedInt64Array x) {
	if (x.is_empty()) {
		return 0;
	}

	return uint64_t(x[0]);
}

// Int64 returns the int64 representation of x.
// If x cannot be represented in an int64, the result is undefined.
int64_t BigInt::Int64() const {
	int64_t v = int64_t(nat_low64(_abs));
	if (_neg) {
		v = -v;
	}

	return v;
}

// Uint64 returns the uint64 representation of x.
// If x cannot be represented in a uint64, the result is undefined.
uint64_t BigInt::Uint64() const {
	return nat_low64(_abs);
}

// IsInt64 reports whether x can be represented as an int64.
bool BigInt::IsInt64() const {
	if (_abs.size() <= 1) {
		const int64_t w = int64_t(nat_low64(_abs));
		return (w >= 0) || (_neg && w == -w);
	}

	return false;
}

// IsUint64 reports whether x can be represented as a uint64.
bool BigInt::IsUint64() const {
	return !_neg && _abs.size() <= 1;
}

// Float64 returns the float64 value nearest x,
// and an indication of any rounding that occurred.
double BigInt::Float64() const {
	const int64_t n = nat_bitLen(_abs);
	if (n == 0) {
		return 0.0;
	}

	// Fast path: no more than 53 significant bits.
	if (n <= 53 || (n < 64 && n - int64_t(nat_trailingZeroBits(_abs)) <= 53)) {
		double f = double(nat_low64(_abs));
		if (_neg) {
			f = -f;
		}
		return f;
	}

	Ref<BigFloat> f;
	f.instantiate();
	f->SetInt(const_cast<BigInt *>(this));

	return f->Float64();
}

BigAccuracy BigInt::Float64Accuracy() const {
	const int64_t n = nat_bitLen(_abs);
	if (n == 0) {
		return ACCURACY_EXACT;
	}

	// Fast path: no more than 53 significant bits.
	if (n <= 53 || (n < 64 && n - int64_t(nat_trailingZeroBits(_abs)) <= 53)) {
		return ACCURACY_EXACT;
	}

	Ref<BigFloat> f;
	f.instantiate();
	f->SetInt(const_cast<BigInt *>(this));

	return f->Float64Accuracy();
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
Ref<BigInt> BigInt::SetString(const String &s, int64_t base) {
	const PackedByteArray buf = s.to_ascii_buffer();
	int64_t i = 0;

	ERR_FAIL_COND_V(scan(buf, i, base).is_null(), nullptr);
	ERR_FAIL_COND_V_MSG(i != buf.size(), nullptr, "expected end of string");

	return this;
}

// SetBytes interprets buf as the bytes of a big-endian unsigned
// integer, sets z to that value, and returns z.
Ref<BigInt> BigInt::SetBytes(const PackedByteArray &buf) {
	nat_setBytes(_abs, buf);
	_neg = false;

	return this;
}

// Bytes returns the absolute value of x as a big-endian byte slice.
//
// To use a fixed length slice, or a preallocated one, use [Int.FillBytes].
PackedByteArray BigInt::Bytes() const {
	PackedByteArray buf;
	return buf.slice(nat_bytes(_abs, buf));
}

// BitLen returns the length of the absolute value of x in bits.
// The bit length of 0 is 0.
int64_t BigInt::BitLen() const {
	return nat_bitLen(_abs);
}

// TrailingZeroBits returns the number of consecutive least significant zero
// bits of |x|.
uint64_t BigInt::TrailingZeroBits() const {
	return nat_trailingZeroBits(_abs);
}

// Exp sets z = x**y mod |m| (i.e. the sign of m is ignored), and returns z.
// If m == nil or m == 0, z = x**y unless y <= 0 then z = 1. If m != 0, y < 0,
// and x and m are not relatively prime, z is unchanged and nil is returned.
//
// Modular exponentiation of inputs of a particular size is not a
// cryptographically constant-time operation.
Ref<BigInt> BigInt::Exp(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &m) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	return exp(x, y, m, false);
}

Ref<BigInt> BigInt::expSlow(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &m) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	return exp(x, y, m, true);
}

Ref<BigInt> BigInt::exp(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &m, bool slow) {
	// See Knuth, volume 2, section 4.6.3.
	PackedInt64Array xWords = x->_abs;
	if (y->_neg) {
		if (m.is_null() || m->_abs.is_empty()) {
			return SetInt64(1);
		}

		// for y < 0: x**y mod m == (x**(-1))**|y| mod m
		Ref<BigInt> inverse;
		inverse.instantiate();
		if (inverse->ModInverse(x, m).is_null()) {
			return nullptr;
		}

		xWords = inverse->_abs;
	}

	PackedInt64Array yWords = y->_abs;

	PackedInt64Array mWords;
	if (m.is_valid()) {
		mWords = m->_abs; // m.abs may be nil for m == 0
	}

	nat_expNN(_abs, xWords, yWords, mWords, slow);
	_neg = !_abs.is_empty() && x->_neg && !yWords.is_empty() && (yWords[0] & 1) == 1; // 0 has no sign
	if (_neg && !mWords.is_empty()) {
		// make modulus result positive
		nat_sub(_abs, mWords, _abs); // z == x**y mod |m| && 0 <= z < |m|
		_neg = false;
	}

	return this;
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
Ref<BigInt> BigInt::GCD(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &a, const Ref<BigInt> &b) {
	ERR_FAIL_NULL_V(*a, nullptr);
	ERR_FAIL_NULL_V(*b, nullptr);

	if (a->_abs.is_empty() || b->_abs.is_empty()) {
		const int64_t lenA = a->_abs.size();
		const int64_t lenB = b->_abs.size();
		const int64_t negA = a->_neg;
		const int64_t negB = b->_neg;
		if (lenA == 0) {
			Set(b);
		} else {
			Set(a);
		}

		_neg = false;
		if (x.is_valid()) {
			if (lenA == 0) {
				x->SetUint64(0);
			} else {
				x->SetUint64(1);
				x->_neg = negA;
			}
		}

		if (y.is_valid()) {
			if (lenB == 0) {
				y->SetUint64(0);
			} else {
				y->SetUint64(1);
				y->_neg = negB;
			}
		}

		return this;
	}

	return lehmerGCD(x, y, a, b);
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
void BigInt::lehmerSimulate(const Ref<BigInt> &A, const Ref<BigInt>&B, uint64_t &u0, uint64_t &u1, uint64_t &v0, uint64_t &v1, bool &even) {
	// initialize the digits
	uint64_t a1, a2, u2, v2;

	const int64_t m = B->_abs.size(); // m >= 2
	const int64_t n = A->_abs.size(); // n >= m >= 2

	// extract the top Word of bits from A and B
	uint64_t h = nat_nlz(A->_abs[n - 1]);
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
		const uint64_t q = a1 / a2;
		const uint64_t r = a1 % a2;
		a1 = a2;
		a2 = r;

		const uint64_t u = u1 + q * u2;
		u0 = u1;
		u1 = u2;
		u2 = u;

		const uint64_t v = v1 + q * v2;
		v0 = v1;
		v1 = v2;
		v2 = v;

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
void BigInt::lehmerUpdate(const Ref<BigInt> &A, const Ref<BigInt> &B, const Ref<BigInt> &q, const Ref<BigInt> &r, uint64_t u0, uint64_t u1, uint64_t v0, uint64_t v1, bool even) {
	q->mulW(B, even, v0);
	r->mulW(A, even, u1);
	A->mulW(A, !even, u0);
	B->mulW(B, !even, v1);
	A->Add(A, q);
	B->Add(B, r);
}

// mulW sets z = x * (-?)w
// where the minus sign is present when neg is true.
void BigInt::mulW(const Ref<BigInt> &x, bool neg, uint64_t w) {
	nat_mulAddWW(_abs, x->_abs, w, 0);
	_neg = x->_neg != neg;
}

// euclidUpdate performs a single step of the Euclidean GCD algorithm
// if extended is true, it also updates the cosequence Ua, Ub.
// q and r are used as temporaries; the initial values are ignored.
void BigInt::euclidUpdate(Ref<BigInt> &A, Ref<BigInt> &B, Ref<BigInt> &Ua, Ref<BigInt> &Ub, const Ref<BigInt> &q, Ref<BigInt> &r, bool extended) {
	q->QuoRem(A, B, r);

	if (extended) {
		// Ua, Ub = Ub, Ua-q*Ub
		q->Mul(q, Ub);
		std::swap(Ua, Ub);
		Ub->Sub(Ub, q);
	}

	const Ref<BigInt> t = A;
	A = B;
	B = r;
	r = t;
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
Ref<BigInt> BigInt::lehmerGCD(const Ref<BigInt> &x, const Ref<BigInt> &y, const Ref<BigInt> &a, const Ref<BigInt> &b) {
	Ref<BigInt> A, B, Ua, Ub;

	A.instantiate();
	A->Abs(a);
	B.instantiate();
	B->Abs(b);

	const bool extended = x.is_valid() || y.is_valid();

	if (extended) {
		// Ua (Ub) tracks how many times input a has been accumulated into A (B).
		Ua.instantiate();
		Ua->SetInt64(1);
		Ub.instantiate();
	}

	// temp variables for multiprecision update
	Ref<BigInt> q, r;
	q.instantiate();
	r.instantiate();

	// ensure A >= B
	if (nat_cmp(A->_abs, B->_abs) < 0) {
		std::swap(A, B);
		std::swap(Ua, Ub);
	}

	// loop invariant A >= B
	while (B->_abs.size() > 1) {
		// Attempt to calculate in single-precision using leading words of A and B.
		uint64_t u0, u1, v0, v1;
		bool even;
		lehmerSimulate(A, B, u0, u1, v0, v1, even);

		// multiprecision Step
		if (v0 != 0) {
			// Simulate the effect of the single-precision steps using the cosequences.
			// A = u0*A + v0*B
			// B = u1*A + v1*B
			lehmerUpdate(A, B, q, r, u0, u1, v0, v1, even);

			if (extended) {
				// Ua = u0*Ua + v0*Ub
				// Ub = u1*Ua + v1*Ub
				lehmerUpdate(Ua, Ub, q, r, u0, u1, v0, v1, even);
			}

		} else {
			// Single-digit calculations failed to simulate any quotients.
			// Do a standard Euclidean step.
			euclidUpdate(A, B, Ua, Ub, q, r, extended);
		}
	}

	if (!B->_abs.is_empty()) {
		// extended Euclidean algorithm base case if B is a single Word
		if (A->_abs.size() > 1) {
			// A is longer than a single Word, so one update is needed.
			euclidUpdate(A, B, Ua, Ub, q, r, extended);
		}
		if (!B->_abs.is_empty()) {
			// A and B are both a single Word.
			uint64_t aWord = A->_abs[0];
			uint64_t bWord = B->_abs[0];
			if (extended) {
				uint64_t ua = 1, ub = 0;
				uint64_t va = 0, vb = 1;
				bool even = true;
				while (bWord != 0) {
					const uint64_t qWord = aWord / bWord;
					const uint64_t rWord = aWord % bWord;
					aWord = bWord;
					bWord = rWord;

					const uint64_t u = ub;
					ub = ua + qWord * ub;
					ua = u;

					const uint64_t v = vb;
					vb = va + qWord * vb;
					va = v;

					even = !even;
				}

				Ua->mulW(Ua, !even, ua);
				Ub->mulW(Ub, even, va);
				Ua->Add(Ua, Ub);
			} else {
				while (bWord != 0) {
					const uint64_t temp = bWord;
					bWord = aWord % bWord;
					aWord = temp;
				}
			}
			A->_abs[0] = aWord;
		}
	}

	const bool negA = a->_neg;
	if (y.is_valid()) {
		// avoid aliasing b needed in the division below
		B->Set(b);
		// y = (z - a*x)/b
		y->Mul(a, Ua); // y can safely alias a
		if (negA) {
			y->_neg = !y->_neg;
		}
		y->Sub(A, y);
		y->Div(y, B);
	}

	if (x.is_valid()) {
		x->Set(Ua);
		if (negA) {
			x->_neg = !x->_neg;
		}
	}

	return Set(A);
}

// Rand sets z to a pseudo-random number in [0, n) and returns z.
//
// As this uses the [math/rand] package, it must not be used for
// security-sensitive work. Use [crypto/rand.Int] instead.
Ref<BigInt> BigInt::Rand(const Callable &rnd, const Ref<BigInt> &n) {
	ERR_FAIL_NULL_V(*n, nullptr);

	// z.neg is not modified before the if check, because z and n might alias.
	if (n->_neg || n->_abs.is_empty()) {
		_neg = false;
		_abs.clear();
		return this;
	}

	_neg = false;
	nat_random(_abs, rnd, n->_abs, nat_bitLen(n->_abs));
	return this;
}

// ModInverse sets z to the multiplicative inverse of g in the ring ℤ/nℤ
// and returns z. If g and n are not relatively prime, g has no multiplicative
// inverse in the ring ℤ/nℤ.  In this case, z is unchanged and the return value
// is nil. If n == 0, a division-by-zero run-time panic occurs.
Ref<BigInt> BigInt::ModInverse(const Ref<BigInt> &g, const Ref<BigInt> &n) {
	ERR_FAIL_NULL_V(*g, nullptr);
	ERR_FAIL_NULL_V(*n, nullptr);

	// GCD expects parameters a and b to be > 0.
	Ref<BigInt> n0 = n;
	if (n->_neg) {
		n0.instantiate();
		n0->Neg(n);
	}
	Ref<BigInt> g0 = g;
	if (g->_neg) {
		g0.instantiate();
		g0->Neg(g);
	}

	Ref<BigInt> d, x;
	d.instantiate();
	x.instantiate();
	d->GCD(x, nullptr, g0, n0);

	// if and only if d==1, g and n are relatively prime
	if (d->Cmp(*intOne) != 0) {
		return nullptr;
	}

	// x and y are such that g*x + n*y = 1, therefore x is the inverse element,
	// but it may be negative, so convert to the range 0 <= z < |n|
	if (x->_neg) {
		Add(x, n0);
	} else {
		Set(x);
	}

	return this;
}

void nat_modInverse(PackedInt64Array &z, PackedInt64Array g, PackedInt64Array n) {
	// TODO(rsc): ModInverse should be implemented in terms of this function.
	Ref<BigInt> zi, gi, ni;
	zi.instantiate();
	zi->_set_abs(z);
	gi.instantiate();
	gi->_set_abs(g);
	ni.instantiate();
	ni->_set_abs(n);
	zi->ModInverse(gi, ni);
	z = zi->_get_abs();
}

// Jacobi returns the Jacobi symbol (x/y), either +1, -1, or 0.
// The y argument must be an odd integer.
int BigInt::Jacobi(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, 0);
	ERR_FAIL_NULL_V(*y, 0);
	ERR_FAIL_COND_V_MSG(y->_abs.is_empty() || (y->_abs[0] & 1) == 0, 0, vformat("invalid 2nd argument to BigInt.Jacobi: need odd integer but got %s", y));

	// We use the formulation described in chapter 2, section 2.4,
	// "The Yacas Book of Algorithms":
	// http://yacas.sourceforge.net/Algo.book.pdf

	Ref<BigInt> a, b, c;
	a.instantiate();
	b.instantiate();
	c.instantiate();
	a->Set(x);
	b->Set(y);

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

		if (a->_abs.is_empty()) {
			return 0;
		}

		a->Mod(a, b);
		if (a->_abs.is_empty()) {
			return 0;
		}

		// a > 0

		// handle factors of 2 in 'a'
		const uint64_t s = nat_trailingZeroBits(a->_abs);
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
Ref<BigInt> BigInt::modSqrt3Mod4Prime(const Ref<BigInt> &x, const Ref<BigInt> &p) {
	Ref<BigInt> e;
	e.instantiate();
	e->Add(p, *intOne);  // e = p + 1
	e->Rsh(e, 2);        // e = (p + 1) / 4
	return Exp(x, e, p); // z = x^e mod p
}

// modSqrt5Mod8Prime uses Atkin's observation that 2 is not a square mod p
//
//	alpha ==  (2*a)^((p-5)/8)    mod p
//	beta  ==  2*a*alpha^2        mod p  is a square root of -1
//	b     ==  a*alpha*(beta-1)   mod p  is a square root of a
//
// to calculate the square root of any quadratic residue mod p quickly for 5
// mod 8 primes.
Ref<BigInt> BigInt::modSqrt5Mod8Prime(const Ref<BigInt> &x, const Ref<BigInt> &p) {
	// p == 5 mod 8 implies p = e*8 + 5
	// e is the quotient and 5 the remainder on division by 8
	Ref<BigInt> e, tx, alpha, beta;
	e.instantiate();
	tx.instantiate();
	alpha.instantiate();
	beta.instantiate();
	e->Rsh(p, 3); // e = (p - 5) / 8
	tx->Lsh(x, 1); // tx = 2*x
	alpha->Exp(tx, e, p);
	beta->Mul(alpha, alpha);
	beta->Mod(beta, p);
	beta->Mul(beta, tx);
	beta->Mod(beta, p);
	beta->Sub(beta, *intOne);
	beta->Mul(beta, x);
	beta->Mod(beta, p);
	beta->Mul(beta, alpha);
	return Mod(beta, p);
}

// modSqrtTonelliShanks uses the Tonelli-Shanks algorithm to find the square
// root of a quadratic residue modulo any prime.
Ref<BigInt> BigInt::modSqrtTonelliShanks(const Ref<BigInt> &x, const Ref<BigInt> &p) {
	// Break p-1 into s*2^e such that s is odd.
	Ref<BigInt> s;
	s.instantiate();
	s->Sub(p, *intOne);

	uint64_t e = nat_trailingZeroBits(s->_abs);
	s->Rsh(s, e);

	// find some non-square n
	Ref<BigInt> n;
	n.instantiate();
	n->SetInt64(2);
	while (Jacobi(n, p) != -1) {
		n->Add(n, *intOne);
	}

	// Core of the Tonelli-Shanks algorithm. Follows the description in
	// section 6 of "Square roots from 1; 24, 51, 10 to Dan Shanks" by Ezra
	// Brown:
	// https://www.maa.org/sites/default/files/pdf/upload_library/22/Polya/07468342.di020786.02p0470a.pdf
	Ref<BigInt> y, b, g, t;
	y.instantiate();
	b.instantiate();
	g.instantiate();
	t.instantiate();

	y->Add(s, *intOne);
	y->Rsh(y, 1);
	y->Exp(x, y, p); // y = x^((s+1)/2)
	b->Exp(x, s, p); // b = x^s
	g->Exp(n, s, p); // g = n^s

	uint64_t r = e;
	while (true) {
		// find the least m such that ord_p(b) = 2^m
		uint64_t m = 0;
		t->Set(b);
		while (t->Cmp(*intOne) != 0) {
			t->Mul(t, t)->Mod(t, p);
			m++;
		}

		if (m == 0) {
			return Set(y);
		}

		t->SetInt64(0)->SetBit(t, int64_t(r - m - 1), 1)->Exp(g, t, p);
		// t = g^(2^(r-m-1)) mod p
		g->Mul(t, t)->Mod(g, p); // g = g^(2^(r-m)) mod p
		y->Mul(y, t)->Mod(y, p);
		b->Mul(b, g)->Mod(b, p);
		r = m;
	}
}

// ModSqrt sets z to a square root of x mod p if such a square root exists, and
// returns z. The modulus p must be an odd prime. If x is not a square mod p,
// ModSqrt leaves z unchanged and returns nil. This function panics if p is
// not an odd integer, its behavior is undefined if p is odd but not prime.
Ref<BigInt> BigInt::ModSqrt(const Ref<BigInt> &x, const Ref<BigInt> &p) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*p, nullptr);

	const int j = Jacobi(x, p);
	if (j == -1) {
		return nullptr; // x is not a square mod p
	}

	if (j == 0) {
		return SetInt64(0);
	}

	Ref<BigInt> x0 = x;
	if (x->_neg || x->Cmp(p) >= 0) { // ensure 0 <= x < p
		x0.instantiate();
		x0->Mod(x, p);
	}

	if (p->_abs[0] % 4 == 3) {
		// Check whether p is 3 mod 4, and if so, use the faster algorithm.
		return modSqrt3Mod4Prime(x0, p);
	}

	if (p->_abs[0] % 8 == 5) {
		// Check whether p is 5 mod 8, use Atkin's algorithm.
		return modSqrt5Mod8Prime(x0, p);
	}

	// Otherwise, use Tonelli-Shanks.
	return modSqrtTonelliShanks(x0, p);
}

// Lsh sets z = x << n and returns z.
Ref<BigInt> BigInt::Lsh(const Ref<BigInt> &x, uint64_t n) {
	ERR_FAIL_NULL_V(*x, nullptr);

	nat_lsh(_abs, x->_abs, n);
	_neg = x->_neg;

	return this;
}

// Rsh sets z = x >> n and returns z.
Ref<BigInt> BigInt::Rsh(const Ref<BigInt> &x, uint64_t n) {
	ERR_FAIL_NULL_V(*x, nullptr);

	if (x->_neg) {
		// (-x) >> s == ^(x-1) >> s == ^((x-1) >> s) == -(((x-1) >> s) + 1)
		nat_sub(_abs, x->_abs, *natOne); // no underflow because |x| > 0
		nat_rsh(_abs, _abs, n);
		nat_add(_abs, _abs, *natOne);
		_neg = true; // z cannot be zero if x is negative

		return this;
	}

	nat_rsh(_abs, x->_abs, n);
	_neg = x->_neg;

	return this;
}

// Bit returns the value of the i'th bit of x. That is, it
// returns (x>>i)&1. The bit index i must be >= 0.
uint64_t BigInt::Bit(int64_t i) const {
	if (i == 0) {
		// optimization for common case: odd/even test of x
		if (!_abs.is_empty()) {
			return uint64_t(_abs[0] & 1); // bit 0 is same for -x
		}

		return 0;
	}

	ERR_FAIL_COND_V_MSG(i < 0, 0, "negative bit index");

	if (_neg) {
		PackedInt64Array t;
		nat_sub(t, _abs, *natOne);
		return nat_bit(t, uint64_t(i)) ^ 1;
	}

	return nat_bit(_abs, uint64_t(i));
}

// SetBit sets z to x, with x's i'th bit set to b (0 or 1).
// That is,
//   - if b is 1, SetBit sets z = x | (1 << i);
//   - if b is 0, SetBit sets z = x &^ (1 << i);
//   - if b is not 0 or 1, SetBit will panic.
Ref<BigInt> BigInt::SetBit(const Ref<BigInt> &x, int64_t i, uint64_t b) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_COND_V_MSG(i < 0, nullptr, "negative bit index");
	ERR_FAIL_COND_V_MSG(b > 1, nullptr, "set bit is not 0 or 1");

	if (x->_neg) {
		nat_sub(_abs, x->_abs, *natOne);
		nat_setBit(_abs, _abs, uint64_t(i), b ^ 1);
		nat_add(_abs, _abs, *natOne);
		_neg = !_abs.is_empty();

		return this;
	}

	nat_setBit(_abs, x->_abs, uint64_t(i), b);
	_neg = false;

	return this;
}

// And sets z = x & y and returns z.
Ref<BigInt> BigInt::And(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x->_neg == y->_neg) {
		if (x->_neg) {
			// (-x) & (-y) == ^(x-1) & ^(y-1) == ^((x-1) | (y-1)) == -(((x-1) | (y-1)) + 1)
			PackedInt64Array x1, y1;
			nat_sub(x1, x->_abs, *natOne);
			nat_sub(y1, y->_abs, *natOne);
			nat_or(_abs, x1, y1);
			nat_add(_abs, _abs, *natOne);
			_neg = true; // z cannot be zero if x and y are negative
			return this;
		}

		// x & y == x & y
		nat_and(_abs, x->_abs, y->_abs);
		_neg = false;
		return this;
	}

	// x.neg != y.neg
	if (x->_neg) {
		return And(y, x); // & is symmetric
	}

	// x & (-y) == x & ^(y-1) == x &^ (y-1)
	PackedInt64Array y1;
	nat_sub(y1, y->_abs, *natOne);
	nat_andNot(_abs, x->_abs, y1);
	_neg = false;
	return this;
}

// AndNot sets z = x &^ y and returns z.
Ref<BigInt> BigInt::AndNot(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x->_neg == y->_neg) {
		if (x->_neg) {
			// (-x) &^ (-y) == ^(x-1) &^ ^(y-1) == ^(x-1) & (y-1) == (y-1) &^ (x-1)
			PackedInt64Array x1, y1;
			nat_sub(x1, x->_abs, *natOne);
			nat_sub(y1, y->_abs, *natOne);
			nat_andNot(_abs, y1, x1);
			_neg = false;
			return this;
		}

		// x &^ y == x &^ y
		nat_andNot(_abs, x->_abs, y->_abs);
		_neg = false;
		return this;
	}

	if (x->_neg) {
		// (-x) &^ y == ^(x-1) &^ y == ^(x-1) & ^y == ^((x-1) | y) == -(((x-1) | y) + 1)
		PackedInt64Array x1;
		nat_sub(x1, x->_abs, *natOne);
		nat_or(_abs, x1, y->_abs);
		nat_add(_abs, _abs, *natOne);
		_neg = true; // z cannot be zero if x is negative and y is positive
		return this;
	}

	// x &^ (-y) == x &^ ^(y-1) == x & (y-1)
	PackedInt64Array y1;
	nat_sub(y1, y->_abs, *natOne);
	nat_and(_abs, x->_abs, y1);
	_neg = false;
	return this;
}

// Or sets z = x | y and returns z.
Ref<BigInt> BigInt::Or(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x->_neg == y->_neg) {
		if (x->_neg) {
			// (-x) | (-y) == ^(x-1) | ^(y-1) == ^((x-1) & (y-1)) == -(((x-1) & (y-1)) + 1)
			PackedInt64Array x1, y1;
			nat_sub(x1, x->_abs, *natOne);
			nat_sub(y1, y->_abs, *natOne);
			nat_and(_abs, x1, y1);
			nat_add(_abs, _abs, *natOne);
			_neg = true; // z cannot be zero if x and y are negative
			return this;
		}

		// x | y == x | y
		nat_or(_abs, x->_abs, y->_abs);
		_neg = false;
		return this;
	}

	// x.neg != y.neg
	if (x->_neg) {
		return Or(y, x); // | is symmetric
	}

	// x | (-y) == x | ^(y-1) == ^((y-1) &^ x) == -(^((y-1) &^ x) + 1)
	PackedInt64Array y1;
	nat_sub(y1, y->_abs, *natOne);
	nat_andNot(_abs, y1, x->_abs);
	nat_add(_abs, _abs, *natOne);
	_neg = true; // z cannot be zero if one of x or y is negative
	return this;
}

// Xor sets z = x ^ y and returns z.
Ref<BigInt> BigInt::Xor(const Ref<BigInt> &x, const Ref<BigInt> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x->_neg == y->_neg) {
		if (x->_neg) {
			// (-x) ^ (-y) == ^(x-1) ^ ^(y-1) == (x-1) ^ (y-1)
			PackedInt64Array x1, y1;
			nat_sub(x1, x->_abs, *natOne);
			nat_sub(y1, y->_abs, *natOne);
			nat_xor(_abs, x1, y1);
			_neg = false;
			return this;
		}

		// x ^ y == x ^ y
		nat_xor(_abs, x->_abs, y->_abs);
		_neg = false;
		return this;
	}

	// x.neg != y.neg
	if (x->_neg) {
		return Xor(y, x); // ^ is symmetric
	}

	// x ^ (-y) == x ^ ^(y-1) == ^(x ^ (y-1)) == -((x ^ (y-1)) + 1)
	PackedInt64Array y1;
	nat_sub(y1, y->_abs, *natOne);
	nat_xor(_abs, x->_abs, y1);
	nat_add(_abs, _abs, *natOne);
	_neg = true; // z cannot be zero if only one of x or y is negative
	return this;
}

// Not sets z = ^x and returns z.
Ref<BigInt> BigInt::Not(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	if (x->_neg) {
		// ^(-x) == ^(^(x-1)) == x-1
		nat_sub(_abs, x->_abs, *natOne);
		_neg = false;
		return this;
	}

	// ^x == -x-1 == -(x+1)
	nat_add(_abs, x->_abs, *natOne);
	_neg = true; // z cannot be zero if x is positive
	return this;
}

// Sqrt sets z to ⌊√x⌋, the largest integer such that z² ≤ x, and returns z.
// It panics if x is negative.
Ref<BigInt> BigInt::Sqrt(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_COND_V_MSG(x->_neg, nullptr, "square root of negative number");

	_neg = false;
	nat_sqrt(_abs, x->_abs);
	return this;
}
