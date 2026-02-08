// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_rat.h"
#include "godot_big_naturals.h"

// This file implements multi-precision rational numbers.

// NewRat creates a new [Rat] with numerator a and denominator b.
Ref<BigRat> BigRat::make(int64_t p_a, int64_t p_b) {
	Ref<BigRat> r;
	r.instantiate();
	return r->SetFrac64(p_a, p_b);
}

// SetFloat64 sets z to exactly f and returns z.
// If f is not finite, SetFloat returns nil.
Ref<BigRat> BigRat::SetFloat64(double f) {
	constexpr uint64_t expMask = (1LLU << 11) - 1;
	const uint64_t bits = std::bit_cast<uint64_t>(f);
	uint64_t mantissa = bits & ((1LLU << 52) - 1);
	int64_t exp = int64_t((bits >> 52) & expMask);
	if (exp == expMask) {
		// non-finite
		return nullptr;
	} else if (exp == 0) {
		// denormal
		exp -= 1022;
	} else {
		// normal
		mantissa |= (1LLU << 52);
		exp -= 1023;
	}

	int64_t shift = 52 - exp;

	// Optimization (?): partially pre-normalise.
	while ((mantissa & 1) == 0 && shift > 0) {
		mantissa >>= 1;
		shift--;
	}

	_a->SetUint64(mantissa);
	_a->_neg = f < 0.0;
	_b->Set(*intOne);

	if (shift > 0) {
		_b->Lsh(_b, uint64_t(shift));
	} else {
		_a->Lsh(_a, uint64_t(-shift));
	}

	return norm();
}

// quotToFloat32 returns the non-negative float32 value
// nearest to the quotient a/b, using round-to-even in
// halfway cases. It does not mutate its arguments.
// Preconditions: b is non-zero; a and b have no common factors.
void nat_quotToFloat32(PackedInt64Array a, PackedInt64Array b, float &f, bool &exact) {
	// float size in bits
	static constexpr uint64_t Fsize = 32;

	// mantissa
	static constexpr uint64_t Msize = 23;
	static constexpr uint64_t Msize1 = Msize + 1; // incl. implicit 1
	static constexpr uint64_t Msize2 = Msize1 + 1;

	// exponent
	static constexpr uint64_t Esize = Fsize - Msize1;
	static constexpr uint64_t Ebias = (1LLU << (Esize - 1)) - 1;
	static constexpr uint64_t Emin = 1 - Ebias;
	static constexpr uint64_t Emax = Ebias;

	// TODO(adonovan): specialize common degenerate cases: 1.0, integers.
	const int64_t alen = nat_bitLen(a);
	if (alen == 0) {
		f = 0;
		exact = true;
		return;
	}

	const int64_t blen = nat_bitLen(b);
	CRASH_COND_MSG(blen == 0, "division by zero");

	// 1. Left-shift A or B such that quotient A/B is in [1<<Msize1, 1<<(Msize2+1)
	// (Msize2 bits if A < B when they are left-aligned, Msize2+1 bits if A >= B).
	// This is 2 or 3 more than the float32 mantissa field width of Msize:
	// - the optional extra bit is shifted away in step 3 below.
	// - the high-order 1 is omitted in "normal" representation;
	// - the low-order 1 will be used during rounding then discarded.
	int64_t exp = alen - blen;
	PackedInt64Array a2, b2;
	nat_set(a2, a);
	nat_set(b2, b);

	{
		const int64_t shift = int64_t(Msize2) - exp;
		if (shift > 0) {
			nat_lsh(a2, a2, uint64_t(shift));
		} else if (shift < 0) {
			nat_lsh(b2, b2, uint64_t(-shift));
		}
	}

	// 2. Compute quotient and remainder (q, r).  NB: due to the
	// extra shift, the low-order bit of q is logically the
	// high-order bit of r.
	PackedInt64Array q;
	nat_div(a2, b2, q, a2); // (recycle a2)
	uint32_t mantissa = nat_low32(q);
	bool haveRem = !a2.is_empty(); // mantissa&1 && !haveRem => remainder is exactly half

	// 3. If quotient didn't fit in Msize2 bits, redo division by b2<<1
	// (in effect---we accomplish this incrementally).
	if ((mantissa >> Msize2) == 1) {
		if ((mantissa & 1) == 1) {
			haveRem = true;
		}
		mantissa >>= 1;
		exp++;
	}

	CRASH_COND_MSG((mantissa >> Msize1) != 1, vformat("expected exactly %d bits of result", Msize2));

	// 4. Rounding.
	if (Emin - Msize <= exp && exp <= Emin) {
		// Denormal case; lose 'shift' bits of precision.
		const uint64_t shift = Emin - uint64_t(exp - 1); // [1..Esize1)
		const uint64_t lostbits = mantissa & ((1LLU << shift) - 1);
		haveRem = haveRem || lostbits != 0;
		mantissa >>= shift;
		exp = 2 - Ebias; // == exp + shift
	}

	// Round q using round-half-to-even.
	exact = !haveRem;
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

	f = std::ldexpf(float(mantissa), exp - Msize1);
	if (!Math::is_finite(f)) {
		exact = false;
	}
}

// quotToFloat64 returns the non-negative float64 value
// nearest to the quotient a/b, using round-to-even in
// halfway cases. It does not mutate its arguments.
// Preconditions: b is non-zero; a and b have no common factors.
void nat_quotToFloat64(PackedInt64Array a, PackedInt64Array b, double &f, bool &exact) {
	// float size in bits
	static constexpr uint64_t Fsize = 64;

	// mantissa
	static constexpr uint64_t Msize = 52;
	static constexpr uint64_t Msize1 = Msize + 1; // incl. implicit 1
	static constexpr uint64_t Msize2 = Msize1 + 1;

	// exponent
	static constexpr uint64_t Esize = Fsize - Msize1;
	static constexpr uint64_t Ebias = (1LLU << (Esize - 1)) - 1;
	static constexpr uint64_t Emin = 1 - Ebias;
	static constexpr uint64_t Emax = Ebias;

	// TODO(adonovan): specialize common degenerate cases: 1.0, integers.
	const int64_t alen = nat_bitLen(a);
	if (alen == 0) {
		f = 0;
		exact = true;
		return;
	}

	const int64_t blen = nat_bitLen(b);
	CRASH_COND_MSG(blen == 0, "division by zero");

	// 1. Left-shift A or B such that quotient A/B is in [1<<Msize1, 1<<(Msize2+1)
	// (Msize2 bits if A < B when they are left-aligned, Msize2+1 bits if A >= B).
	// This is 2 or 3 more than the float64 mantissa field width of Msize:
	// - the optional extra bit is shifted away in step 3 below.
	// - the high-order 1 is omitted in "normal" representation;
	// - the low-order 1 will be used during rounding then discarded.
	int64_t exp = alen - blen;
	PackedInt64Array a2, b2;
	nat_set(a2, a);
	nat_set(b2, b);

	{
		const int64_t shift = int64_t(Msize2) - exp;
		if (shift > 0) {
			nat_lsh(a2, a2, uint64_t(shift));
		} else if (shift < 0) {
			nat_lsh(b2, b2, uint64_t(-shift));
		}
	}

	// 2. Compute quotient and remainder (q, r).  NB: due to the
	// extra shift, the low-order bit of q is logically the
	// high-order bit of r.
	PackedInt64Array q;
	nat_div(a2, b2, q, a2); // (recycle a2)
	uint64_t mantissa = nat_low64(q);
	bool haveRem = !a2.is_empty(); // mantissa&1 && !haveRem => remainder is exactly half

	// 3. If quotient didn't fit in Msize2 bits, redo division by b2<<1
	// (in effect---we accomplish this incrementally).
	if ((mantissa >> Msize2) == 1) {
		if ((mantissa & 1) == 1) {
			haveRem = true;
		}
		mantissa >>= 1;
		exp++;
	}

	CRASH_COND_MSG((mantissa >> Msize1) != 1, vformat("expected exactly %d bits of result", Msize2));

	// 4. Rounding.
	if (Emin - Msize <= exp && exp <= Emin) {
		// Denormal case; lose 'shift' bits of precision.
		const uint64_t shift = Emin - uint64_t(exp - 1); // [1..Esize1)
		const uint64_t lostbits = mantissa & ((1LLU << shift) - 1);
		haveRem = haveRem || lostbits != 0;
		mantissa >>= shift;
		exp = 2 - Ebias; // == exp + shift
	}

	// Round q using round-half-to-even.
	exact = !haveRem;
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

	f = std::ldexp(double(mantissa), exp - Msize1);
	if (!Math::is_finite(f)) {
		exact = false;
	}
}

// Float32 returns the nearest float32 value for x and a bool indicating
// whether f represents x exactly. If the magnitude of x is too large to
// be represented by a float32, f is an infinity and exact is false.
// The sign of f always matches the sign of x, even if f == 0.
float BigRat::Float32() const {
	PackedInt64Array b = _b->_abs;
	if (b.is_empty()) {
		b = *natOne;
	}

	float f;
	bool exact;
	nat_quotToFloat32(_a->_abs, b, f, exact);

	if (_a->_neg) {
		f = -f;
	}

	return f;
}

bool BigRat::Float32Exact() const {
	PackedInt64Array b = _b->_abs;
	if (b.is_empty()) {
		b = *natOne;
	}

	float f;
	bool exact;
	nat_quotToFloat32(_a->_abs, b, f, exact);

	return exact;
}

// Float64 returns the nearest float64 value for x and a bool indicating
// whether f represents x exactly. If the magnitude of x is too large to
// be represented by a float64, f is an infinity and exact is false.
// The sign of f always matches the sign of x, even if f == 0.
double BigRat::Float64() const {
	PackedInt64Array b = _b->_abs;
	if (b.is_empty()) {
		b = *natOne;
	}

	double f;
	bool exact;
	nat_quotToFloat64(_a->_abs, b, f, exact);

	if (_a->_neg) {
		f = -f;
	}

	return f;
}

bool BigRat::Float64Exact() const {
	PackedInt64Array b = _b->_abs;
	if (b.is_empty()) {
		b = *natOne;
	}

	double f;
	bool exact;
	nat_quotToFloat64(_a->_abs, b, f, exact);

	return exact;
}

// SetFrac sets z to a/b and returns z.
// If b == 0, SetFrac panics.
Ref<BigRat> BigRat::SetFrac(const Ref<BigInt> &a, const Ref<BigInt> &b) {
	ERR_FAIL_NULL_V(*a, nullptr);
	ERR_FAIL_NULL_V(*b, nullptr);

	PackedInt64Array babs = b->_abs;
	ERR_FAIL_COND_V_MSG(babs.is_empty(), nullptr, "division by zero");

	_a->_neg = a->_neg != b->_neg;
	nat_set(_a->_abs, a->_abs);
	nat_set(_b->_abs, babs);

	return norm();
}

// SetFrac64 sets z to a/b and returns z.
// If b == 0, SetFrac64 panics.
Ref<BigRat> BigRat::SetFrac64(int64_t a, int64_t b) {
	ERR_FAIL_COND_V_MSG(b == 0, nullptr, "division by zero");

	_a->SetInt64(a);

	if (b < 0) {
		b = -b;
		_a->_set_neg(!_a->_is_neg());
	}

	PackedInt64Array babs = _b->_get_abs();
	nat_setUint64(babs, uint64_t(b));
	_b->_set_abs(babs);

	return norm();
}

// SetInt sets z to x (by making a copy of x) and returns z.
Ref<BigRat> BigRat::SetInt(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	_a->Set(x);
	nat_setWord(_b->_abs, 1);

	return this;
}

// SetInt64 sets z to x and returns z.
Ref<BigRat> BigRat::SetInt64(int64_t x) {
	_a->SetInt64(x);
	nat_setWord(_b->_abs, 1);

	return this;
}

// SetUint64 sets z to x and returns z.
Ref<BigRat> BigRat::SetUint64(uint64_t x) {
	_a->SetUint64(x);
	nat_setWord(_b->_abs, 1);

	return this;
}

// Set sets z to x (by making a copy of x) and returns z.
Ref<BigRat> BigRat::Set(const Ref<BigRat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	if (this != *x) {
		_a->Set(x->_a);
		_b->Set(x->_b);
	}

	if (_b->_abs.is_empty()) {
		nat_setWord(_b->_abs, 1);
	}

	return this;
}

// Abs sets z to |x| (the absolute value of x) and returns z.
Ref<BigRat> BigRat::Abs(const Ref<BigRat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_a->_neg = false;

	return this;
}

// Neg sets z to -x and returns z.
Ref<BigRat> BigRat::Neg(const Ref<BigRat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_a->_neg = !_a->_abs.is_empty() && !_a->_neg; // 0 has no sign

	return this;
}

// Inv sets z to 1/x and returns z.
// If x == 0, Inv panics.
Ref<BigRat> BigRat::Inv(const Ref<BigRat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_COND_V_MSG(x->_a->_abs.is_empty(), nullptr, "division by zero");

	Set(x);
	std::swap(_a->_abs, _b->_abs);

	return this;
}

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x == 0;
//   - +1 if x > 0.
int BigRat::Sign() const {
	return _a->Sign();
}

// IsInt reports whether the denominator of x is 1.
bool BigRat::IsInt() const {
	return _b->_abs.is_empty() || nat_cmp(_b->_abs, *natOne) == 0;
}

// Num returns the numerator of x; it may be <= 0.
// The result is a reference to x's numerator; it
// may change if a new value is assigned to x, and vice versa.
// The sign of the numerator corresponds to the sign of x.
Ref<BigInt> BigRat::Num() const {
	return _a;
}

// Denom returns the denominator of x; it is always > 0.
// The result is a reference to x's denominator, unless
// x is an uninitialized (zero value) [Rat], in which case
// the result is a new [Int] of value 1. (To initialize x,
// any operation that sets x will do, including x.Set(x).)
// If the result is a reference to x's denominator it
// may change if a new value is assigned to x, and vice versa.
Ref<BigInt> BigRat::Denom() const {
	// Note that x.b.neg is guaranteed false.
	if (_b->_abs.is_empty()) {
		// Note: If this proves problematic, we could
		//       panic instead and require the Rat to
		//       be explicitly initialized.
		return BigInt::make(1);
	}

	return _b;
}

Ref<BigRat> BigRat::norm() {
	if (_a->_get_abs().is_empty()) {
		// z == 0; normalize sign and denominator
		_a->_neg = false;

		nat_setWord(_b->_abs, 1);
		_b->_neg = false;

		return this;
	}

	if (_b->_get_abs().is_empty()) {
		// z is integer; normalize denominator
		nat_setWord(_b->_abs, 1);
		_b->_neg = false;

		return this;
	}

	// z is fraction; normalize numerator and denominator
	const bool neg = _a->_neg;
	_a->_neg = false;
	_b->_neg = false;

	Ref<BigInt> f;
	f.instantiate();
	f->lehmerGCD(nullptr, nullptr, _a, _b);

	if (f->Cmp(*intOne) != 0) {
		PackedInt64Array remainder;
		nat_div(_a->_abs, f->_abs, _a->_abs, remainder);
		nat_div(_b->_abs, f->_abs, _b->_abs, remainder);
	}

	_a->_neg = neg;

	return this;
}

// mulDenom sets z to the denominator product x*y (by taking into
// account that 0 values for x or y must be interpreted as 1) and
// returns z.
void nat_mulDenom(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y) {
	if (x.is_empty() && y.is_empty()) {
		nat_setWord(z, 1);
		return;
	}

	if (x.is_empty()) {
		nat_set(z, y);
		return;
	}

	if (y.is_empty()) {
		nat_set(z, x);
		return;
	}

	nat_mul(z, x, y);
}

// scaleDenom sets z to the product x*f.
// If f == 0 (zero value of denominator), z is set to (a copy of) x.
void BigInt::scaleDenom(const Ref<BigInt> &x, PackedInt64Array f) {
	if (f.is_empty()) {
		Set(x);
		return;
	}

	nat_mul(_abs, x->_abs, f);
	_neg = x->_neg;
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y;
//   - +1 if x > y.
int BigRat::Cmp(const Ref<BigRat> &y) const {
	ERR_FAIL_NULL_V(*y, 0);

	Ref<BigInt> a, b;
	a.instantiate();
	b.instantiate();

	a->scaleDenom(_a, y->_b->_abs);
	b->scaleDenom(y->_a, _b->_abs);

	return a->Cmp(b);
}

// Add sets z to the sum x+y and returns z.
Ref<BigRat> BigRat::Add(const Ref<BigRat> &x, const Ref<BigRat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	Ref<BigInt> a1, a2;
	a1.instantiate();
	a2.instantiate();
	a1->scaleDenom(x->_a, y->_b->_abs);
	a2->scaleDenom(y->_a, x->_b->_abs);

	_a->Add(a1, a2);
	nat_mulDenom(_b->_abs, x->_b->_abs, y->_b->_abs);

	return norm();
}

// Sub sets z to the difference x-y and returns z.
Ref<BigRat> BigRat::Sub(const Ref<BigRat> &x, const Ref<BigRat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	Ref<BigInt> a1, a2;
	a1.instantiate();
	a2.instantiate();

	a1->scaleDenom(x->_a, y->_b->_abs);
	a2->scaleDenom(y->_a, x->_b->_abs);

	_a->Sub(a1, a2);
	nat_mulDenom(_b->_abs, x->_b->_abs, y->_b->_abs);

	return norm();
}

// Mul sets z to the product x*y and returns z.
Ref<BigRat> BigRat::Mul(const Ref<BigRat> &x, const Ref<BigRat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	if (x == y) {
		// a squared Rat is positive and can't be reduced (no need to call norm())
		_a->_neg = false;
		nat_sqr(_a->_abs, x->_a->_abs);

		if (x->_b->_abs.is_empty()) {
			nat_setWord(_b->_abs, 1);
		} else {
			nat_sqr(_b->_abs, x->_b->_abs);
		}

		return this;
	}

	_a->Mul(x->_a, y->_a);
	nat_mulDenom(_b->_abs, x->_b->_abs, y->_b->_abs);

	return norm();
}

// Quo sets z to the quotient x/y and returns z.
// If y == 0, Quo panics.
Ref<BigRat> BigRat::Quo(const Ref<BigRat> &x, const Ref<BigRat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

	ERR_FAIL_COND_V_MSG(y->_a->_abs.is_empty(), nullptr, "division by zero");

	Ref<BigInt> a, b;
	a.instantiate();
	b.instantiate();

	a->scaleDenom(x->_a, y->_b->_abs);
	b->scaleDenom(y->_a, x->_b->_abs);

	_a->_abs = a->_abs;
	_b->_abs = b->_abs;
	_a->_neg = a->_neg != b->_neg;

	return norm();
}
