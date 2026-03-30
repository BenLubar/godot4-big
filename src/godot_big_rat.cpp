// This file is ported from src/math/big/rat.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements multi-precision rational numbers.

#include "godot_big_rat.h"
#include "godot_big_int.h"

using namespace godot;

// A Rat represents a quotient a/b of arbitrary precision.
// The zero value for a Rat represents the value 0.
//
// Operations always take pointer arguments (*Rat) rather
// than Rat values, and each unique Rat value requires
// its own unique *Rat pointer. To "copy" a Rat value,
// an existing (or newly allocated) Rat must be set to
// a new value using the [Rat.Set] method; shallow copies
// of Rats are not supported and may lead to errors.

extern Ref<BigInt> *intOne;

// NewRat creates a new [Rat] with numerator a and denominator b.
Ref<BigRat> BigRat::NewRat(int64_t p_a, int64_t p_b) {
	ERR_FAIL_COND_V(p_b == 0, nullptr);

	Ref<BigRat> r;
	r.instantiate();
	r->SetFrac64(p_a, p_b);
	return r;
}

// SetFloat64 sets z to exactly f and returns z.
// If f is not finite, SetFloat returns nil.
Error BigRat::SetFloat64(double p_f) {
	constexpr uint64_t expMask = 1LLU << 11 - 1;
	const uint64_t bits = std::bit_cast<uint64_t>(p_f);
	uint64_t mantissa = bits & (1LLU << 52 - 1);
	int32_t exp = int32_t((bits >> 52) & expMask);
	ERR_FAIL_COND_V_MSG(exp == expMask, ERR_INVALID_PARAMETER, "float is non-finite");

	if (exp == 0) {
		// denormal
		exp -= 1022;
	} else {
		// normal
		mantissa |= 1LLU << 52;
		exp -= 1023;
	}

	int32_t shift = 52 - exp;

	// Optimization (?): partially pre-normalise.
	while ((mantissa & 1) == 0 && shift > 0) {
		mantissa >>= 1;
		shift--;
	}

	_a.setUint64(mantissa);
	_neg = p_f < 0;
	_b.setUint64(1);
	if (shift > 0) {
		_b.lsh(_b, uint64_t(shift));
	} else {
		_a.rsh(_a, uint64_t(-shift));
	}

	_norm();
	return OK;
}

// quotToFloat32 returns the non-negative float32 value
// nearest to the quotient a/b, using round-to-even in
// halfway cases. It does not mutate its arguments.
// Preconditions: b is non-zero; a and b have no common factors.
static Pair<float, bool> quotToFloat32(BigNat a, BigNat b) {
	// float size in bits
	static constexpr int64_t Fsize = 32;

	// mantissa
	static constexpr int64_t Msize  = 23;
	static constexpr int64_t Msize1 = Msize + 1; // incl. implicit 1
	static constexpr int64_t Msize2 = Msize1 + 1;

	// exponent
	static constexpr int64_t Esize = Fsize - Msize1;
	static constexpr int64_t Ebias = 1LL << (Esize - 1) - 1;
	static constexpr int64_t Emin  = 1 - Ebias;
	static constexpr int64_t Emax  = Ebias;

	// TODO(adonovan): specialize common degenerate cases: 1.0, integers.
	const int64_t alen = a.bitLen();
	if (alen == 0) {
		return {0.0f, true};
	}

	const int64_t blen = b.bitLen();
	CRASH_COND_MSG(blen == 0, "division by zero");

	// 1. Left-shift A or B such that quotient A/B is in [1<<Msize1, 1<<(Msize2+1)
	// (Msize2 bits if A < B when they are left-aligned, Msize2+1 bits if A >= B).
	// This is 2 or 3 more than the float64 mantissa field width of Msize:
	// - the optional extra bit is shifted away in step 3 below.
	// - the high-order 1 is omitted in "normal" representation;
	// - the low-order 1 will be used during rounding then discarded.
	int64_t exp = alen - blen;
	BigNat a2, b2;
	a2.set(a);
	b2.set(b);
	const int64_t shift = Msize2 - exp;
	if (shift > 0) {
		a2.lsh(a2, uint64_t(shift));
	} else if (shift < 0) {
		b2.lsh(b2, uint64_t(-shift));
	}

	// 2. Compute quotient and remainder (q, r).  NB: due to the
	// extra shift, the low-order bit of q is logically the
	// high-order bit of r.
	BigNat q;
	BigNat &r = a2; // (recycle a2)
	q.div(r, a2, b2);
	uint64_t mantissa = q.low32();
	bool haveRem = !r.array.is_empty(); // mantissa&1 && !haveRem => remainder is exactly half

	// 3. If quotient didn't fit in Msize2 bits, redo division by b2<<1
	// (in effect---we accomplish this incrementally).
	if ((mantissa >> Msize2) == 1) {
		if ((mantissa & 1) == 1) {
			haveRem = true;
		}
		mantissa >>= 1;
		exp++;
	}
	CRASH_COND((mantissa >> Msize1) != 1); // expected exactly Msize2 bits of result

	// 4. Rounding.
	if (Emin - Msize <= exp && exp <= Emin) {
		// Denormal case; lose 'shift' bits of precision.
		const uint64_t shift = uint64_t(Emin - (exp - 1)); // [1..Esize1)
		const uint64_t lostbits = mantissa & ((1LLU << shift) - 1);
		haveRem = haveRem || lostbits != 0;
		mantissa >>= shift;
		exp = 2 - Ebias; // == exp + shift
	}
	// Round q using round-half-to-even.
	bool exact = !haveRem;
	if ((mantissa & 1) != 0) {
		exact = false;
		if (haveRem || (mantissa & 2) != 0) {
			mantissa++;
			if (mantissa >= (1LLU << Msize2)) {
				// Complete rollover 11...1 => 100...0, so shift is safe
				mantissa >>= 1;
				exp++;
			}
		}
	}
	mantissa >>= 1; // discard rounding bit.  Mantissa now scaled by 1<<Msize1.

	const float f = std::ldexpf(float(mantissa), exp - Msize1);
	if (Math::is_inf(f)) {
		exact = false;
	}

	return {f, exact};
}

// quotToFloat64 returns the non-negative float64 value
// nearest to the quotient a/b, using round-to-even in
// halfway cases. It does not mutate its arguments.
// Preconditions: b is non-zero; a and b have no common factors.
static Pair<double, bool> quotToFloat64(BigNat a, BigNat b) {
	// float size in bits
	static constexpr int64_t Fsize = 64;

	// mantissa
	static constexpr int64_t Msize  = 52;
	static constexpr int64_t Msize1 = Msize + 1; // incl. implicit 1
	static constexpr int64_t Msize2 = Msize1 + 1;

	// exponent
	static constexpr int64_t Esize = Fsize - Msize1;
	static constexpr int64_t Ebias = 1LL << (Esize - 1) - 1;
	static constexpr int64_t Emin  = 1 - Ebias;
	static constexpr int64_t Emax  = Ebias;

	// TODO(adonovan): specialize common degenerate cases: 1.0, integers.
	const int64_t alen = a.bitLen();
	if (alen == 0) {
		return {0.0, true};
	}

	const int64_t blen = b.bitLen();
	CRASH_COND_MSG(blen == 0, "division by zero");

	// 1. Left-shift A or B such that quotient A/B is in [1<<Msize1, 1<<(Msize2+1)
	// (Msize2 bits if A < B when they are left-aligned, Msize2+1 bits if A >= B).
	// This is 2 or 3 more than the float64 mantissa field width of Msize:
	// - the optional extra bit is shifted away in step 3 below.
	// - the high-order 1 is omitted in "normal" representation;
	// - the low-order 1 will be used during rounding then discarded.
	int64_t exp = alen - blen;
	BigNat a2, b2;
	a2.set(a);
	b2.set(b);
	const int64_t shift = Msize2 - exp;
	if (shift > 0) {
		a2.lsh(a2, uint64_t(shift));
	} else if (shift < 0) {
		b2.lsh(b2, uint64_t(-shift));
	}

	// 2. Compute quotient and remainder (q, r).  NB: due to the
	// extra shift, the low-order bit of q is logically the
	// high-order bit of r.
	BigNat q;
	BigNat &r = a2; // (recycle a2)
	q.div(r, a2, b2);
	uint64_t mantissa = q.low64();
	bool haveRem = !r.array.is_empty(); // mantissa&1 && !haveRem => remainder is exactly half

	// 3. If quotient didn't fit in Msize2 bits, redo division by b2<<1
	// (in effect---we accomplish this incrementally).
	if ((mantissa >> Msize2) == 1) {
		if ((mantissa & 1) == 1) {
			haveRem = true;
		}
		mantissa >>= 1;
		exp++;
	}
	CRASH_COND((mantissa >> Msize1) != 1); // expected exactly Msize2 bits of result

	// 4. Rounding.
	if (Emin - Msize <= exp && exp <= Emin) {
		// Denormal case; lose 'shift' bits of precision.
		const uint64_t shift = uint64_t(Emin - (exp - 1)); // [1..Esize1)
		const uint64_t lostbits = mantissa & ((1LLU << shift) - 1);
		haveRem = haveRem || lostbits != 0;
		mantissa >>= shift;
		exp = 2 - Ebias; // == exp + shift
	}
	// Round q using round-half-to-even.
	bool exact = !haveRem;
	if ((mantissa & 1) != 0) {
		exact = false;
		if (haveRem || (mantissa & 2) != 0) {
			mantissa++;
			if (mantissa >= (1LLU << Msize2)) {
				// Complete rollover 11...1 => 100...0, so shift is safe
				mantissa >>= 1;
				exp++;
			}
		}
	}
	mantissa >>= 1; // discard rounding bit.  Mantissa now scaled by 1<<Msize1.

	const double f = std::ldexp(double(mantissa), exp - Msize1);
	if (Math::is_inf(f)) {
		exact = false;
	}

	return {f, exact};
}

// Float32 returns the nearest float32 value for x and a bool indicating
// whether f represents x exactly. If the magnitude of x is too large to
// be represented by a float32, f is an infinity and exact is false.
// The sign of f always matches the sign of x, even if f == 0.
Pair<float, bool> BigRat::Float32() const {
	Pair<float, bool> f = quotToFloat32(_a, _b);
	if (_neg) {
		f.first = -f.first;
	}
	return f;
}

// Float64 returns the nearest float64 value for x and a bool indicating
// whether f represents x exactly. If the magnitude of x is too large to
// be represented by a float64, f is an infinity and exact is false.
// The sign of f always matches the sign of x, even if f == 0.
Pair<double, bool> BigRat::Float64() const {
	Pair<double, bool> f = quotToFloat64(_a, _b);
	if (_neg) {
		f.first = -f.first;
	}
	return f;
}

// SetFrac sets z to a/b and returns z.
// If b == 0, SetFrac panics.
Error BigRat::SetFrac(const Ref<BigInt> &p_a, const Ref<BigInt> &p_b) {
	ERR_FAIL_NULL_V(*p_a, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_b, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_b->_abs.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	_neg = p_a->_neg != p_b->_neg;
	_a.set(p_a->_abs);
	_b.set(p_b->_abs);

	_norm();
	emit_changed();
	return OK;
}

// SetFrac64 sets z to a/b and returns z.
// If b == 0, SetFrac64 panics.
Error BigRat::SetFrac64(int64_t p_a, int64_t p_b) {
	ERR_FAIL_COND_V_MSG(p_b == 0, ERR_INVALID_PARAMETER, "division by zero");

	_neg = (p_a < 0) != (p_b < 0);
	_a.setUint64(uint64_t(p_a < 0 ? -p_a : p_a));
	_b.setUint64(uint64_t(p_b < 0 ? -p_b : p_b));

	_norm();
	emit_changed();
	return OK;
}

// SetInt sets z to x (by making a copy of x) and returns z.
void BigRat::SetInt(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_neg = p_x->_neg;
	_a.set(p_x->_abs);
	_b.setUint64(1);

	emit_changed();
}

// SetInt64 sets z to x and returns z.
void BigRat::SetInt64(int64_t p_x) {
	_neg = p_x < 0;
	_a.setUint64(uint64_t(p_x < 0 ? -p_x : p_x));
	_b.setUint64(1);

	emit_changed();
}

// SetUint64 sets z to x and returns z.
void BigRat::SetUint64(uint64_t p_x) {
	_neg = false;
	_a.setUint64(p_x);
	_b.setUint64(1);

	emit_changed();
}

// Set sets z to x (by making a copy of x) and returns z.
void BigRat::Set(const Ref<BigRat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_neg = p_x->_neg;
	_a.set(p_x->_a);
	_b.set(p_x->_b);

	emit_changed();
}

// Abs sets z to |x| (the absolute value of x) and returns z.
void BigRat::Abs(const Ref<BigRat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_neg = false;
	_a.set(p_x->_a);
	_b.set(p_x->_b);

	emit_changed();
}

// Neg sets z to -x and returns z.
void BigRat::Neg(const Ref<BigRat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	_neg = !p_x->_a.array.is_empty() && !p_x->_neg; // 0 has no sign
	_a.set(p_x->_a);
	_b.set(p_x->_b);

	emit_changed();
}

// Inv sets z to 1/x and returns z.
// If x == 0, Inv panics.
godot::Error BigRat::Inv(const Ref<BigRat> &p_x) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_x->_a.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	_neg = p_x->_neg;

	BigNat temp = p_x->_a;
	_a = p_x->_b;
	_b = std::move(temp);

	emit_changed();
	return OK;
}

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x == 0;
//   - +1 if x > 0.
int BigRat::Sign() const {
	if (_a.array.is_empty()) {
		return 0;
	}
	if (_neg) {
		return -1;
	}
	return 1;
}

// IsInt reports whether the denominator of x is 1.
bool BigRat::IsInt() const {
	return _b.cmp(BigNat{{1}}) == 0;
}

// Num returns the numerator of x; it may be <= 0.
// The result is a reference to x's numerator; it
// may change if a new value is assigned to x, and vice versa.
// The sign of the numerator corresponds to the sign of x.
void BigRat::Num(const Ref<BigInt> &r_a) const {
	ERR_FAIL_NULL(*r_a);

	r_a->_neg = _neg;
	r_a->_abs = _a;
	r_a->emit_changed();
}

// Denom returns the denominator of x; it is always > 0.
// The result is a reference to x's denominator, unless
// x is an uninitialized (zero value) [Rat], in which case
// the result is a new [Int] of value 1. (To initialize x,
// any operation that sets x will do, including x.Set(x).)
// If the result is a reference to x's denominator it
// may change if a new value is assigned to x, and vice versa.
void BigRat::Denom(const Ref<BigInt> &r_b) const {
	ERR_FAIL_NULL(*r_b);

	r_b->_neg = false;
	r_b->_abs = _b;
	r_b->emit_changed();
}

void BigRat::_norm() {
	// unlike the Go version of this code, denominator is initialized to 1
	DEV_ASSERT(!_b.array.is_empty());

	if (_a.array.is_empty()) {
		// z == 0; normalize sign and denominator
		_neg = false;
		_b.setUint64(1);
		return;
	}

	// z is fraction; normalize numerator and denominator
	Ref<BigInt> f, A, B;
	f.instantiate();
	A.instantiate();
	B.instantiate();

	A->_abs = _a;
	B->_abs = _b;
	f->_lehmerGCD(nullptr, nullptr, A, B);

	if (f->Cmp(*intOne) != 0) {
		BigNat r;
		_a.div(r, _a, f->_abs);
		_b.div(r, _b, f->_abs);
	}
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y;
//   - +1 if x > y.
int BigRat::Cmp(const Ref<BigRat> &p_y) const {
	ERR_FAIL_NULL_V(*p_y, 0);

	Ref<BigInt> a, b, n, d;
	a.instantiate();
	b.instantiate();
	n.instantiate();
	d.instantiate();

	Num(n);
	p_y->Denom(d);
	a->Mul(n, d);

	p_y->Num(n);
	Denom(d);
	b->Mul(n, d);

	return a->Cmp(b);
}

// Add sets z to the sum x+y and returns z.
void BigRat::Add(const Ref<BigRat> &p_x, const Ref<BigRat> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	Ref<BigInt> a1, a2, n, d;
	a1.instantiate();
	a2.instantiate();
	n.instantiate();
	d.instantiate();

	p_x->Num(n);
	p_y->Denom(d);
	a1->Mul(n, d);

	p_y->Num(n);
	p_x->Denom(d);
	a2->Mul(n, d);

	n->Add(a1, a2);
	_neg = n->_neg;
	_a = n->_abs;

	_b.mul(p_x->_b, p_y->_b);

	_norm();
	emit_changed();
}

// Sub sets z to the difference x-y and returns z.
void BigRat::Sub(const Ref<BigRat> &p_x, const Ref<BigRat> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	Ref<BigInt> a1, a2, n, d;
	a1.instantiate();
	a2.instantiate();
	n.instantiate();
	d.instantiate();

	p_x->Num(n);
	p_y->Denom(d);
	a1->Mul(n, d);

	p_y->Num(n);
	p_x->Denom(d);
	a2->Mul(n, d);

	n->Sub(a1, a2);
	_neg = n->_neg;
	_a = n->_abs;

	_b.mul(p_x->_b, p_y->_b);

	_norm();
	emit_changed();
}

// Mul sets z to the product x*y and returns z.
void BigRat::Mul(const Ref<BigRat> &p_x, const Ref<BigRat> &p_y) {
	ERR_FAIL_NULL(*p_x);
	ERR_FAIL_NULL(*p_y);

	if (p_x == p_y) {
		// a squared Rat is positive and can't be reduced (no need to call norm())
		_neg = false;
		_a.sqr(p_x->_a);
		_b.sqr(p_x->_b);

		emit_changed();
		return;
	}

	_a.mul(p_x->_a, p_y->_a);
	_neg = !_a.array.is_empty() && p_x->_neg != p_y->_neg;
	_b.mul(p_x->_b, p_y->_b);

	_norm();
	emit_changed();
}

// Quo sets z to the quotient x/y and returns z.
// If y == 0, Quo panics.
Error BigRat::Quo(const Ref<BigRat> &p_x, const Ref<BigRat> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_y->_a.array.is_empty(), ERR_INVALID_PARAMETER, "division by zero");

	Ref<BigInt> a, b, n, d;
	a.instantiate();
	b.instantiate();
	n.instantiate();
	d.instantiate();

	p_x->Num(n);
	p_y->Denom(d);
	a->Mul(n, d);

	p_y->Num(n);
	p_x->Denom(d);
	b->Mul(n, d);

	_a = a->_abs;
	_b = b->_abs;
	_neg = a->_neg != b->_neg;

	_norm();
	emit_changed();
	return OK;
}
