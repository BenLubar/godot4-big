// This file is ported from src/math/big/float.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2014 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"

#include "godot_big_int.h"
#include "godot_big_rat.h"

#include <cfloat>

using namespace godot;

// This file implements multi-precision floating-point numbers.
// Like in the GNU MPFR library (https://www.mpfr.org/), operands
// can be of mixed precision. Unlike MPFR, the rounding mode is
// not specified with each operation, but with each operand. The
// rounding mode of the result operand determines the rounding
// mode of an operation. This is a from-scratch implementation.

// A nonzero finite Float represents a multi-precision floating point number
//
//	sign × mantissa × 2**exponent
//
// with 0.5 <= mantissa < 1.0, and MinExp <= exponent <= MaxExp.
// A Float may also be zero (+0, -0) or infinite (+Inf, -Inf).
// All Floats are ordered, and the ordering of two Floats x and y
// is defined by x.Cmp(y).
//
// Each Float value also has a precision, rounding mode, and accuracy.
// The precision is the maximum number of mantissa bits available to
// represent the value. The rounding mode specifies how a result should
// be rounded to fit into the mantissa bits, and accuracy describes the
// rounding error with respect to the exact result.
//
// Unless specified otherwise, all operations (including setters) that
// specify a *Float variable for the result (usually via the receiver
// with the exception of [Float.MantExp]), round the numeric result according
// to the precision and rounding mode of the result variable.
//
// If the provided result precision is 0 (see below), it is set to the
// precision of the argument with the largest precision value before any
// rounding takes place, and the rounding mode remains unchanged. Thus,
// uninitialized Floats provided as result arguments will have their
// precision set to a reasonable value determined by the operands, and
// their mode is the zero value for RoundingMode (ToNearestEven).
//
// By setting the desired precision to 24 or 53 and using matching rounding
// mode (typically [ToNearestEven]), Float operations produce the same results
// as the corresponding float32 or float64 IEEE 754 arithmetic for operands
// that correspond to normal (i.e., not denormal) float32 or float64 numbers.
// Exponent underflow and overflow lead to a 0 or an Infinity for different
// values than IEEE 754 because Float exponents have a much larger range.
//
// The zero (uninitialized) value for a Float is ready to use and represents
// the number +0.0 exactly, with precision 0 and rounding mode [ToNearestEven].
//
// Operations always take pointer arguments (*Float) rather
// than Float values, and each unique Float value requires
// its own unique *Float pointer. To "copy" a Float value,
// an existing (or newly allocated) Float must be set to
// a new value using the [Float.Set] method; shallow copies
// of Floats are not supported and may lead to errors.

// NewFloat allocates and returns a new [Float] set to x,
// with precision 53 and rounding mode [ToNearestEven].
// NewFloat panics with [ErrNaN] if x is a NaN.
Ref<BigFloat> BigFloat::NewFloat(double p_x) {
	ERR_FAIL_COND_V(Math::is_nan(p_x), nullptr);

	Ref<BigFloat> f{ memnew(BigFloat) };
	f->SetFloat64(p_x);
	return f;
}

// Internal representation: The mantissa bits x.mant of a nonzero finite
// Float x are stored in a nat slice long enough to hold up to x.prec bits;
// the slice may (but doesn't have to) be shorter if the mantissa contains
// trailing 0 bits. x.mant is normalized if the msb of x.mant == 1 (i.e.,
// the msb is shifted all the way "to the left"). Thus, if the mantissa has
// trailing 0 bits or x.prec is not a multiple of the Word size _W,
// x.mant[0] has trailing zero bits. The msb of the mantissa corresponds
// to the value 0.5; the exponent x.exp shifts the binary point as needed.
//
// A zero or non-finite Float x ignores x.mant and x.exp.
//
// x                 form      neg      mant         exp
// ----------------------------------------------------------
// ±0                zero      sign     -            -
// 0 < |x| < +Inf    finite    sign     mantissa     exponent
// ±Inf              inf       sign     -            -

// SetPrec sets z's precision to prec and returns the (possibly) rounded
// value of z. Rounding occurs according to z's rounding mode if the mantissa
// cannot be represented in prec bits without loss of precision.
// SetPrec(0) maps all finite values to ±0; infinite values remain unchanged.
// If prec > [MaxPrec], it is set to [MaxPrec].
void BigFloat::SetPrec(uint64_t p_prec) {
	_acc = ACC_EXACT; // optimistically assume no rounding is needed

	// special case
	if (p_prec == 0) {
		if (_prec != 0) {
			_prec = 0;
			if (_form == FORM_FINITE) {
				// truncate z to 0
				_acc = _neg ? ACC_ABOVE : ACC_BELOW;
				_form = FORM_ZERO;
			}
			emit_changed();
		}
		return;
	}

	// general case
	if (p_prec > static_cast<uint64_t>(MAX_PREC)) {
		p_prec = MAX_PREC;
	}
	const uint32_t old = _prec;
	_prec = static_cast<uint32_t>(p_prec);
	if (_prec < old) {
		_round(0);
	}
	if (_prec != old) {
		emit_changed();
	}
}

// SetMode sets z's rounding mode to mode and returns an exact z.
// z remains unchanged otherwise.
// z.SetMode(z.Mode()) is a cheap way to set z's accuracy to [Exact].
void BigFloat::SetMode(RoundingMode p_mode) {
	if (_mode != p_mode || _acc != ACC_EXACT) {
		_mode = p_mode;
		_acc = ACC_EXACT;
		emit_changed();
	}
}

// Prec returns the mantissa precision of x in bits.
// The result may be 0 for |x| == 0 and |x| == Inf.
uint32_t BigFloat::Prec() const {
	return _prec;
}

// MinPrec returns the minimum precision required to represent x exactly
// (i.e., the smallest prec before x.SetPrec(prec) would start rounding x).
// The result is 0 for |x| == 0 and |x| == Inf.
uint32_t BigFloat::MinPrec() const {
	if (_form != FORM_FINITE) {
		return 0;
	}

	return static_cast<uint64_t>(_mant.array.size() * 64) - _mant.trailingZeroBits();
}

// Mode returns the rounding mode of x.
BigFloat::RoundingMode BigFloat::Mode() const {
	return _mode;
}

// Acc returns the accuracy of x produced by the most recent
// operation, unless explicitly documented otherwise by that
// operation.
BigAccuracy BigFloat::Acc() const {
	return _acc;
}

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x is ±0;
//   - +1 if x > 0.
int BigFloat::Sign() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		return 0;
	}

	if (_neg) {
		return -1;
	}

	return 1;
}

// MantExp breaks x into its mantissa and exponent components
// and returns the exponent. If a non-nil mant argument is
// provided its value is set to the mantissa of x, with the
// same precision and rounding mode as x. The components
// satisfy x == mant × 2**exp, with 0.5 <= |mant| < 1.0.
// Calling MantExp with a nil argument is an efficient way to
// get the exponent of the receiver.
//
// Special cases are:
//
//	(  ±0).MantExp(mant) = 0, with mant set to   ±0
//	(±Inf).MantExp(mant) = 0, with mant set to ±Inf
//
// x and mant may be the same in which case x is set to its
// mantissa value.
int32_t BigFloat::MantExp(const Ref<BigFloat> &r_mant) const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	int32_t exp = 0;
	if (_form == FORM_FINITE) {
		exp = _exp;
	}

	if (r_mant.is_valid()) {
		r_mant->Copy(const_cast<BigFloat *>(this));
		if (r_mant->_form == FORM_FINITE) {
			r_mant->_exp = 0;
		}
	}

	return exp;
}

void BigFloat::_setExpAndRound(int64_t p_exp, uint64_t p_sbit) {
	if (p_exp < MIN_EXP) {
		// underflow
		_acc = _neg ? ACC_ABOVE : ACC_BELOW;
		_form = FORM_ZERO;
		return;
	}

	if (p_exp > MAX_EXP) {
		// overflow
		_acc = _neg ? ACC_BELOW : ACC_ABOVE;
		_form = FORM_INF;
		return;
	}

	_form = FORM_FINITE;
	_exp = static_cast<int32_t>(p_exp);
	_round(p_sbit);
}

// SetMantExp sets z to mant × 2**exp and returns z.
// The result z has the same precision and rounding mode
// as mant. SetMantExp is an inverse of [Float.MantExp] but does
// not require 0.5 <= |mant| < 1.0. Specifically, for a
// given x of type *[Float], SetMantExp relates to [Float.MantExp]
// as follows:
//
//	mant := new(Float)
//	new(Float).SetMantExp(mant, x.MantExp(mant)).Cmp(x) == 0
//
// Special cases are:
//
//	z.SetMantExp(  ±0, exp) =   ±0
//	z.SetMantExp(±Inf, exp) = ±Inf
//
// z and mant may be the same in which case z's exponent
// is set to exp.
void BigFloat::SetMantExp(const Ref<BigFloat> &p_mant, int64_t p_exp) {
	ERR_FAIL_NULL(*p_mant);

#ifdef DEBUG_ENABLED
	_validate();
	p_mant->_validate();
#endif

	Copy(p_mant);

	if (_form == FORM_FINITE) {
		// 0 < |mant| < +Inf
		_setExpAndRound(static_cast<int64_t>(_exp) + p_exp, 0);
	}

	emit_changed();
}

// Signbit reports whether x is negative or negative zero.
bool BigFloat::Signbit() const {
	return _neg;
}

// IsInf reports whether x is +Inf or -Inf.
bool BigFloat::IsInf() const {
	return _form == FORM_INF;
}

// IsInt reports whether x is an integer.
// ±Inf values are not integers.
bool BigFloat::IsInt() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	// special cases
	if (_form != FORM_FINITE) {
		return _form == FORM_ZERO;
	}

	// x.form == finite
	if (_exp <= 0) {
		return false;
	}

	// x.exp > 0
	return _prec <= static_cast<uint32_t>(_exp) || MinPrec() <= static_cast<uint64_t>(_exp); // not enough bits for fractional mantissa
}

// debugging support
void BigFloat::_validate() const {
#ifdef DEBUG_ENABLED
	if (_form == FORM_FINITE) {
		const int64_t m = _mant.array.size();
		DEV_ASSERT(m != 0); // nonzero finite number with empty mantissa
		constexpr BigWord msb = 1LLU << (64 - 1);
		DEV_ASSERT((_mant[m - 1] & msb) != 0); // msb not set in last word
		DEV_ASSERT(_prec != 0); // zero precision finite number
	}
#endif
}

// round rounds z according to z.mode to z.prec bits and sets z.acc accordingly.
// sbit must be 0 or 1 and summarizes any "sticky bit" information one might
// have before calling round. z's mantissa must be normalized (with the msb set)
// or empty.
//
// CAUTION: The rounding modes [ToNegativeInf], [ToPositiveInf] are affected by the
// sign of z. For correct rounding, the sign of z must be set correctly before
// calling round.
void BigFloat::_round(uint64_t p_sbit) {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	_acc = ACC_EXACT;
	if (_form != FORM_FINITE) {
		// ±0 or ±Inf => nothing left to do
		return;
	}
	// z.form == finite && len(z.mant) > 0
	// m > 0 implies z.prec > 0 (checked by validate)

	uint32_t m = static_cast<uint32_t>(_mant.array.size()); // present mantissa length in words
	uint32_t bits = m * 64; // present mantissa bits; bits > 0
	if (bits <= _prec) {
		// mantissa fits => nothing to do
		return;
	}
	// bits > z.prec

	// Rounding is based on two bits: the rounding bit (rbit) and the
	// sticky bit (sbit). The rbit is the bit immediately before the
	// z.prec leading mantissa bits (the "0.5"). The sbit is set if any
	// of the bits before the rbit are set (the "0.25", "0.125", etc.):
	//
	//   rbit  sbit  => "fractional part"
	//
	//   0     0        == 0
	//   0     1        >  0  , < 0.5
	//   1     0        == 0.5
	//   1     1        >  0.5, < 1.0

	// bits > z.prec: mantissa too large => round
	uint64_t r = static_cast<uint64_t>(bits - _prec - 1); // rounding bit position; r >= 0
	uint64_t rbit = _mant.bit(r) & 1; // rounding bit; be safe and ensure it's a single bit
	// The sticky bit is only needed for rounding ToNearestEven
	// or when the rounding bit is zero. Avoid computation otherwise.
	if (p_sbit == 0 && (rbit == 0 || _mode == TO_NEAREST_EVEN)) {
		p_sbit = _mant.sticky(r);
	}
	p_sbit &= 1; // be safe and ensure it's a single bit

	// cut off extra words
	uint32_t n = (_prec + (64 - 1)) / 64; // mantissa length in words for desired precision
	if (m > n) {
		_mant.array = _mant.array.slice(m - n); // move n last words to front
	}

	// determine number of trailing zero bits (ntz) and compute lsb mask of mantissa's least-significant word
	uint32_t ntz = (n * 64) - _prec; // 0 <= ntz < _W
	BigWord lsb = static_cast<BigWord>(1) << ntz;

	// round if result is inexact
	if ((rbit | p_sbit) != 0) {
		// Make rounding decision: The result mantissa is truncated ("rounded down")
		// by default. Decide if we need to increment, or "round up", the (unsigned)
		// mantissa.
		bool inc = false;
		switch (_mode) {
			case TO_NEGATIVE_INF:
				inc = _neg;
				break;
			case TO_ZERO:
				// nothing to do
				break;
			case TO_NEAREST_EVEN:
				inc = rbit != 0 && (p_sbit != 0 || (_mant[0] & lsb) != 0);
				break;
			case TO_NEAREST_AWAY:
				inc = rbit != 0;
				break;
			case AWAY_FROM_ZERO:
				inc = true;
				break;
			case TO_POSITIVE_INF:
				inc = !_neg;
				break;
		}

		// A positive result (!z.neg) is Above the exact result if we increment,
		// and it's Below if we truncate (Exact results require no rounding).
		// For a negative result (z.neg) it is exactly the opposite.
		_acc = inc != _neg ? ACC_ABOVE : ACC_BELOW;

		if (inc) {
			// add 1 to mantissa
			if (BigNat::addVW(_mant, _mant, lsb) != 0) {
				// mantissa overflow => adjust exponent
				if (_exp >= MAX_EXP) {
					// exponent overflow
					_form = FORM_INF;
					return;
				}

				_exp++;
				// adjust mantissa: divide by 2 to compensate for exponent adjustment
				BigNat::rshVU(_mant, _mant, 1);
				// set msb == carry == 1 from the mantissa overflow above
				constexpr uint64_t msb = 1LLU << (64 - 1);
				_mant[n - 1] |= msb;
			}
		}
	}

	// zero out trailing bits in least-significant word
	_mant[0] &= ~(lsb - 1);

#ifdef DEBUG_ENABLED
	_validate();
#endif
}

void BigFloat::_setBits64(bool p_neg, uint64_t p_x) {
	if (_prec == 0) {
		_prec = 64;
	}

	_acc = ACC_EXACT;
	_neg = p_neg;
	if (p_x == 0) {
		_form = FORM_ZERO;
		emit_changed();
		return;
	}

	// x != 0
	_form = FORM_FINITE;
	const uint64_t s = std::countl_zero(p_x);
	_mant.setUint64(p_x << s);
	_exp = 64 - static_cast<int32_t>(s); // always fits
	if (_prec < 64) {
		_round(0);
	}

	emit_changed();
}

// SetUint64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 64 (and rounding will have
// no effect).
void BigFloat::SetUint64(uint64_t p_x) {
	_setBits64(false, p_x);
}

// SetInt64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 64 (and rounding will have
// no effect).
void BigFloat::SetInt64(int64_t p_x) {
	int64_t u = p_x;
	if (u < 0) {
		u = -u;
	}

	// We cannot simply call z.SetUint64(uint64(u)) and change
	// the sign afterwards because the sign affects rounding.
	_setBits64(p_x < 0, static_cast<uint64_t>(u));
}

// SetFloat64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 53 (and rounding will have
// no effect). SetFloat64 panics with [ErrNaN] if x is a NaN.
Error BigFloat::SetFloat64(double p_x) {
	ERR_FAIL_COND_V(Math::is_nan(p_x), ERR_INVALID_PARAMETER);

	if (_prec == 0) {
		_prec = 53;
	}

	_acc = ACC_EXACT;
	_neg = std::signbit(p_x); // handle -0, -Inf correctly
	if (p_x == 0) {
		_form = FORM_ZERO;
		emit_changed();
		return OK;
	}

	if (Math::is_inf(p_x)) {
		_form = FORM_INF;
		emit_changed();
		return OK;
	}
	// normalized x != 0

	_form = FORM_FINITE;
	int exp = 0;
	double fmant = std::frexp(p_x, &exp); // get normalized mantissa
	_mant.setUint64((1LLU << 63) | (std::bit_cast<uint64_t>(fmant) << 11));
	_exp = static_cast<int32_t>(exp); // always fits
	if (_prec < 53) {
		_round(0);
	}

	emit_changed();
	return OK;
}

// fnorm normalizes mantissa m by shifting it to the left
// such that the msb of the most-significant word (msw) is 1.
// It returns the shift amount. It assumes that len(m) != 0.
int64_t BigNat::fnorm() {
	DEV_ASSERT(!array.is_empty() && array[array.size() - 1] != 0);

	const uint64_t s = std::countl_zero(static_cast<uint64_t>(array[array.size() - 1]));
	if (s > 0) {
		[[maybe_unused]] const BigWord c = lshVU(*this, *this, s);
		DEV_ASSERT(c == 0);
	}

	return static_cast<int64_t>(s);
}

// SetInt sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the larger of x.BitLen()
// or 64 (and rounding will have no effect).
void BigFloat::SetInt(const Ref<BigInt> &p_x) {
	ERR_FAIL_NULL(*p_x);

	// TODO(gri) can be more efficient if z.prec > 0
	// but small compared to the size of x, or if there
	// are many trailing 0's.
	uint32_t bits = static_cast<uint32_t>(p_x->BitLen());
	if (_prec == 0) {
		_prec = Math::max(bits, 64u);
	}
	_acc = ACC_EXACT;
	_neg = p_x->_neg;
	if (p_x->_abs.array.is_empty()) {
		_form = FORM_ZERO;
		emit_changed();
		return;
	}
	// x != 0
	_mant.set(p_x->_abs);
	_mant.fnorm();
	_setExpAndRound(static_cast<int64_t>(bits), 0);
	emit_changed();
}

// SetRat sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the largest of a.BitLen(),
// b.BitLen(), or 64; with x = a/b.
void BigFloat::SetRat(const Ref<BigRat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	Ref<BigInt> x{ memnew(BigInt) };
	p_x->Num(x);

	if (p_x->IsInt()) {
		SetInt(x);
		return;
	}

	Ref<BigFloat> a{ memnew(BigFloat) };
	Ref<BigFloat> b{ memnew(BigFloat) };

	a->SetInt(x);
	p_x->Denom(x);
	b->SetInt(x);

	if (_prec == 0) {
		_prec = Math::max(a->_prec, b->_prec);
	}

	Quo(a, b);
}

// SetInf sets z to the infinite Float -Inf if signbit is
// set, or +Inf if signbit is not set, and returns z. The
// precision of z is unchanged and the result is always
// [Exact].
void BigFloat::SetInf(bool p_signbit) {
	_acc = ACC_EXACT;
	_form = FORM_INF;
	_neg = p_signbit;
	emit_changed();
}

// Set sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the precision of x
// before setting z (and rounding will have no effect).
// Rounding is performed according to z's precision and rounding
// mode; and z's accuracy reports the result error relative to the
// exact (not rounded) result.
void BigFloat::Set(const Ref<BigFloat> &p_x) {
	ERR_FAIL_NULL(*p_x);

#ifdef DEBUG_ENABLED
	p_x->validate();
#endif

	_acc = ACC_EXACT;
	if (this != *p_x) {
		_form = p_x->_form;
		_neg = p_x->_neg;

		if (p_x->_form == FORM_FINITE) {
			_exp = p_x->_exp;
			_mant.set(p_x->_mant);
		}

		if (_prec == 0) {
			_prec = p_x->_prec;
		} else if (_prec < p_x->_prec) {
			_round(0);
		}

		emit_changed();
	}
}

// Copy sets z to x, with the same precision, rounding mode, and accuracy as x.
// Copy returns z. If x and z are identical, Copy is a no-op.
void BigFloat::Copy(const Ref<BigFloat> &p_x) {
	ERR_FAIL_NULL(*p_x);

#ifdef DEBUG_ENABLED
	p_x->validate();
#endif

	if (this != *p_x) {
		_prec = p_x->_prec;
		_mode = p_x->_mode;
		_acc = p_x->_acc;
		_form = p_x->_form;
		_neg = p_x->_neg;

		if (_form == FORM_FINITE) {
			_mant.set(p_x->_mant);
			_exp = p_x->_exp;
		}

		emit_changed();
	}
}

// msb32 returns the 32 most significant bits of x.
uint32_t BigNat::msb32() const {
	int64_t i = array.size() - 1;
	if (i < 0) {
		return 0;
	}

	DEV_ASSERT(((*this)[i] & (1LLU << (64 - 1))) != 0);

	return static_cast<uint32_t>((*this)[i] >> 32);
}

// msb64 returns the 64 most significant bits of x.
uint64_t BigNat::msb64() const {
	int64_t i = array.size() - 1;
	if (i < 0) {
		return 0;
	}

	DEV_ASSERT(((*this)[i] & (1LLU << (64 - 1))) != 0);

	return (*this)[i];
}

// Uint64 returns the unsigned integer resulting from truncating x
// towards zero. If 0 <= x <= [math.MaxUint64], the result is [Exact]
// if x is an integer and [Below] otherwise.
// The result is (0, [Above]) for x < 0, and ([math.MaxUint64], [Below])
// for x > [math.MaxUint64].
Pair<uint64_t, BigAccuracy> BigFloat::Uint64() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		return { UINT64_C(0), ACC_EXACT };
	}

	if (_form == FORM_INF) {
		if (_neg) {
			return { UINT64_C(0), ACC_ABOVE };
		}

		return { UINT64_MAX, ACC_BELOW };
	}

	if (_neg) {
		return { UINT64_C(0), ACC_ABOVE };
	}

	// 0 < x < +Inf
	if (_exp <= 0) {
		// 0 < x < 1
		return { UINT64_C(0), ACC_BELOW };
	}

	// 1 <= x < Inf
	if (_exp <= 64) {
		// u = trunc(x) fits into a uint64
		const uint64_t u = _mant.msb64() >> (64 - static_cast<uint32_t>(_exp));
		if (MinPrec() <= 64) {
			return { u, ACC_EXACT };
		}

		return { u, ACC_BELOW }; // x truncated
	}

	// x too large
	return { UINT64_MAX, ACC_BELOW };
}

// Int64 returns the integer resulting from truncating x towards zero.
// If [math.MinInt64] <= x <= [math.MaxInt64], the result is [Exact] if x is
// an integer, and [Above] (x < 0) or [Below] (x > 0) otherwise.
// The result is ([math.MinInt64], [Above]) for x < [math.MinInt64],
// and ([math.MaxInt64], [Below]) for x > [math.MaxInt64].
Pair<int64_t, BigAccuracy> BigFloat::Int64() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		return { INT64_C(0), ACC_EXACT };
	}

	if (_form == FORM_INF) {
		if (_neg) {
			return { INT64_MIN, ACC_ABOVE };
		}

		return { INT64_MAX, ACC_BELOW };
	}

	// 0 < |x| < +Inf
	BigAccuracy acc = _neg ? ACC_ABOVE : ACC_BELOW;
	if (_exp <= 0) {
		// 0 < |x| < 1
		return { INT64_C(0), acc };
	}
	// x.exp > 0

	// 1 <= |x| < +Inf
	if (_exp <= 63) {
		// i = trunc(x) fits into an int64 (excluding math.MinInt64)
		int64_t i = static_cast<int64_t>(_mant.msb64() >> (64 - static_cast<uint32_t>(_exp)));
		if (_neg) {
			i = -i;
		}

		if (MinPrec() <= static_cast<uint64_t>(_exp)) {
			return { i, ACC_EXACT };
		}

		return { i, acc }; // x truncated
	}

	if (_neg) {
		// check for special case x == math.MinInt64 (i.e., x == -(0.5 << 64))
		if (_exp == 64 && MinPrec() == 1) {
			acc = ACC_EXACT;
		}

		return { INT64_MIN, acc };
	}

	// x too large
	return { INT64_MAX, ACC_BELOW };
}

// Float32 returns the float32 value nearest to x. If x is too small to be
// represented by a float32 (|x| < [math.SmallestNonzeroFloat32]), the result
// is (0, [Below]) or (-0, [Above]), respectively, depending on the sign of x.
// If x is too large to be represented by a float32 (|x| > [math.MaxFloat32]),
// the result is (+Inf, [Above]) or (-Inf, [Below]), depending on the sign of x.
Pair<float, BigAccuracy> BigFloat::Float32() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		if (_neg) {
			const float z = 0.0f;
			return { -z, ACC_EXACT };
		}

		return { 0.0f, ACC_EXACT };
	}

	if (_form == FORM_INF) {
		if (_neg) {
			return { -HUGE_VALF, ACC_EXACT };
		}

		return { HUGE_VALF, ACC_EXACT };
	}

	// 0 < |x| < +Inf

	static constexpr int32_t fbits = 32; //        float size
	static constexpr int32_t mbits = 23; //        mantissa size (excluding implicit msb)
	static constexpr int32_t ebits = fbits - mbits - 1; //     8  exponent size
	static constexpr int32_t bias = (1 << (ebits - 1)) - 1; //   127  exponent bias
	static constexpr int32_t dmin = 1 - bias - mbits; //  -149  smallest unbiased exponent (denormal)
	static constexpr int32_t emin = 1 - bias; //  -126  smallest unbiased exponent (normal)
	static constexpr int32_t emax = bias; //   127  largest unbiased exponent (normal)

	// Float mantissa m is 0.5 <= m < 1.0; compute exponent e for float32 mantissa.
	int32_t e = _exp - 1; // exponent for normal mantissa m with 1.0 <= m < 2.0

	// Compute precision p for float32 mantissa.
	// If the exponent is too small, we have a denormal number before
	// rounding and fewer than p mantissa bits of precision available
	// (the exponent remains fixed but the mantissa gets shifted right).
	int32_t p = mbits + 1; // precision of normal float
	if (e < emin) {
		// recompute precision
		p = mbits + 1 - emin + e;
		// If p == 0, the mantissa of x is shifted so much to the right
		// that its msb falls immediately to the right of the float32
		// mantissa space. In other words, if the smallest denormal is
		// considered "1.0", for p == 0, the mantissa value m is >= 0.5.
		// If m > 0.5, it is rounded up to 1.0; i.e., the smallest denormal.
		// If m == 0.5, it is rounded down to even, i.e., 0.0.
		// If p < 0, the mantissa value m is <= "0.25" which is never rounded up.
		if (p < 0 /* m <= 0.25 */ || (p == 0 && _mant.sticky(static_cast<uint64_t>((_mant.array.size() * 64) - 1)) == 0) /* m == 0.5 */) {
			// underflow to ±0
			if (_neg) {
				const float z = 0.0f;
				return { -z, ACC_ABOVE };
			}

			return { 0.0f, ACC_BELOW };
		}

		// otherwise, round up
		// We handle p == 0 explicitly because it's easy and because
		// Float.round doesn't support rounding to 0 bits of precision.
		if (p == 0) {
			if (_neg) {
				return { -FLT_TRUE_MIN, ACC_BELOW };
			}
			return { FLT_TRUE_MIN, ACC_ABOVE };
		}
	}
	// p > 0

	// round
	Ref<BigFloat> r{ memnew(BigFloat) };
	r->_prec = static_cast<uint32_t>(p);
	r->Set(const_cast<BigFloat *>(this));
	e = r->_exp - 1;

	// Rounding may have caused r to overflow to ±Inf
	// (rounding never causes underflows to 0).
	// If the exponent is too large, also overflow to ±Inf.
	if (r->_form == FORM_INF || e > emax) {
		// overflow
		if (_neg) {
			return { -HUGE_VALF, ACC_BELOW };
		}

		return { HUGE_VALF, ACC_ABOVE };
	}
	// e <= emax

	// Determine sign, biased exponent, and mantissa.
	uint32_t sign = 0;
	uint32_t bexp = 0;
	uint32_t mant = 0;
	if (_neg) {
		sign = 1U << (fbits - 1);
	}

	// Rounding may have caused a denormal number to
	// become normal. Check again.
	if (e < emin) {
		// denormal number: recompute precision
		// Since rounding may have at best increased precision
		// and we have eliminated p <= 0 early, we know p > 0.
		// bexp == 0 for denormals
		p = mbits + 1 - emin + e;
		mant = r->_mant.msb32() >> static_cast<uint32_t>(fbits - p);
	} else {
		// normal number: emin <= e <= emax
		bexp = static_cast<uint32_t>(e + bias) << mbits;
		mant = (r->_mant.msb32() >> ebits) & ((1 << mbits) - 1); // cut off msb (implicit 1 bit)
	}

	return { std::bit_cast<float>(sign | bexp | mant), r->_acc };
}

// Float64 returns the float64 value nearest to x. If x is too small to be
// represented by a float64 (|x| < [math.SmallestNonzeroFloat64]), the result
// is (0, [Below]) or (-0, [Above]), respectively, depending on the sign of x.
// If x is too large to be represented by a float64 (|x| > [math.MaxFloat64]),
// the result is (+Inf, [Above]) or (-Inf, [Below]), depending on the sign of x.
Pair<double, BigAccuracy> BigFloat::Float64() const {
#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		if (_neg) {
			const double z = 0.0;
			return { -z, ACC_EXACT };
		}

		return { 0.0, ACC_EXACT };
	}

	if (_form == FORM_INF) {
		if (_neg) {
			return { -HUGE_VAL, ACC_EXACT };
		}

		return { HUGE_VAL, ACC_EXACT };
	}

	// 0 < |x| < +Inf

	static constexpr int64_t fbits = 64; //        float size
	static constexpr int64_t mbits = 52; //        mantissa size (excluding implicit msb)
	static constexpr int64_t ebits = fbits - mbits - 1; //    11  exponent size
	static constexpr int64_t bias = (1 << (ebits - 1)) - 1; //  1023  exponent bias
	static constexpr int64_t dmin = 1 - bias - mbits; // -1074  smallest unbiased exponent (denormal)
	static constexpr int64_t emin = 1 - bias; // -1022  smallest unbiased exponent (normal)
	static constexpr int64_t emax = bias; //  1023  largest unbiased exponent (normal)

	// Float mantissa m is 0.5 <= m < 1.0; compute exponent e for float64 mantissa.
	int64_t e = static_cast<int64_t>(_exp) - 1; // exponent for normal mantissa m with 1.0 <= m < 2.0

	// Compute precision p for float64 mantissa.
	// If the exponent is too small, we have a denormal number before
	// rounding and fewer than p mantissa bits of precision available
	// (the exponent remains fixed but the mantissa gets shifted right).
	int64_t p = mbits + 1; // precision of normal float
	if (e < emin) {
		// recompute precision
		p = mbits + 1 - emin + e;
		// If p == 0, the mantissa of x is shifted so much to the right
		// that its msb falls immediately to the right of the float64
		// mantissa space. In other words, if the smallest denormal is
		// considered "1.0", for p == 0, the mantissa value m is >= 0.5.
		// If m > 0.5, it is rounded up to 1.0; i.e., the smallest denormal.
		// If m == 0.5, it is rounded down to even, i.e., 0.0.
		// If p < 0, the mantissa value m is <= "0.25" which is never rounded up.
		if (p < 0 /* m <= 0.25 */ || (p == 0 && _mant.sticky(static_cast<uint64_t>((_mant.array.size() * 64) - 1)) == 0) /* m == 0.5 */) {
			// underflow to ±0
			if (_neg) {
				const double z = 0.0;
				return { -z, ACC_ABOVE };
			}
			return { 0.0, ACC_BELOW };
		}

		// otherwise, round up
		// We handle p == 0 explicitly because it's easy and because
		// Float.round doesn't support rounding to 0 bits of precision.
		if (p == 0) {
			if (_neg) {
				return { -DBL_TRUE_MIN, ACC_BELOW };
			}

			return { DBL_TRUE_MIN, ACC_ABOVE };
		}
	}
	// p > 0

	// round
	Ref<BigFloat> r{ memnew(BigFloat) };
	r->_prec = static_cast<uint32_t>(p);
	r->Set(const_cast<BigFloat *>(this));
	e = static_cast<int64_t>(r->_exp) - 1;

	// Rounding may have caused r to overflow to ±Inf
	// (rounding never causes underflows to 0).
	// If the exponent is too large, also overflow to ±Inf.
	if (r->_form == FORM_INF || e > emax) {
		// overflow
		if (_neg) {
			return { -HUGE_VAL, ACC_BELOW };
		}
		return { HUGE_VAL, ACC_ABOVE };
	}
	// e <= emax

	// Determine sign, biased exponent, and mantissa.
	uint64_t sign = 0;
	uint64_t bexp = 0;
	uint64_t mant = 0;
	if (_neg) {
		sign = 1LLU << (fbits - 1);
	}

	// Rounding may have caused a denormal number to
	// become normal. Check again.
	if (e < emin) {
		// denormal number: recompute precision
		// Since rounding may have at best increased precision
		// and we have eliminated p <= 0 early, we know p > 0.
		// bexp == 0 for denormals
		p = mbits + 1 - emin + e;
		mant = r->_mant.msb64() >> static_cast<uint64_t>(fbits - p);
	} else {
		// normal number: emin <= e <= emax
		bexp = static_cast<uint64_t>(e + bias) << mbits;
		mant = (r->_mant.msb64() >> ebits) & ((1LLU << mbits) - 1); // cut off msb (implicit 1 bit)
	}

	return { std::bit_cast<double>(sign | bexp | mant), r->_acc };
}

// Int returns the result of truncating x towards zero;
// or nil if x is an infinity.
// The result is [Exact] if x.IsInt(); otherwise it is [Below]
// for x > 0, and [Above] for x < 0.
// If a non-nil *[Int] argument z is provided, [Int] stores
// the result in z instead of allocating a new [Int].
BigAccuracy BigFloat::Int(const Ref<BigInt> &r_z) const {
	ERR_FAIL_NULL_V(*r_z, ACC_EXACT);

#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		r_z->SetUint64(0);

		return ACC_EXACT;
	}

	if (_form == FORM_INF) {
		return _neg ? ACC_ABOVE : ACC_BELOW;
	}

	// 0 < |x| < +Inf
	BigAccuracy acc = _neg ? ACC_ABOVE : ACC_BELOW;
	if (_exp <= 0) {
		// 0 < |x| < 1
		r_z->SetUint64(0);

		return acc;
	}
	// x.exp > 0

	// 1 <= |x| < +Inf
	// determine minimum required precision for x
	const uint64_t allBits = static_cast<uint64_t>(_mant.array.size()) * 64;
	const uint64_t exp = static_cast<uint64_t>(_exp);
	if (MinPrec() <= exp) {
		acc = ACC_EXACT;
	}
	// shift mantissa as needed
	r_z->_neg = _neg;
	if (exp > allBits) {
		r_z->_abs.lsh(_mant, exp - allBits);
	} else if (exp < allBits) {
		r_z->_abs.rsh(_mant, allBits - exp);
	} else {
		r_z->_abs.set(_mant);
	}

	r_z->emit_changed();
	return acc;
}

// Rat returns the rational number corresponding to x;
// or nil if x is an infinity.
// The result is [Exact] if x is not an Inf.
// If a non-nil *[Rat] argument z is provided, [Rat] stores
// the result in z instead of allocating a new [Rat].
BigAccuracy BigFloat::Rat(const Ref<BigRat> &r_z) const {
	ERR_FAIL_NULL_V(*r_z, ACC_EXACT);

#ifdef DEBUG_ENABLED
	_validate();
#endif

	if (_form == FORM_ZERO) {
		r_z->SetUint64(0);

		return ACC_EXACT;
	}

	if (_form == FORM_INF) {
		return _neg ? ACC_ABOVE : ACC_BELOW;
	}

	// 0 < |x| < +Inf
	const int32_t allBits = _mant.array.size() * 64;
	// build up numerator and denominator
	r_z->_neg = _neg;
	if (_exp > allBits) {
		r_z->_a.lsh(_mant, static_cast<uint64_t>(_exp - allBits));
		r_z->_b.setUint64(1);
		// z already in normal form
	} else if (_exp < allBits) {
		r_z->_a.set(_mant);
		r_z->_b.setUint64(1);
		r_z->_b.lsh(r_z->_b, static_cast<uint64_t>(allBits - _exp));
		r_z->_norm();
	} else {
		r_z->_a.set(_mant);
		r_z->_b.setUint64(1);
		// z already in normal form
	}

	r_z->emit_changed();
	return ACC_EXACT;
}

// Abs sets z to the (possibly rounded) value |x| (the absolute value of x)
// and returns z.
void BigFloat::Abs(const Ref<BigFloat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	Set(p_x);
	_neg = false;
	emit_changed();
}

// Neg sets z to the (possibly rounded) value of x with its sign negated,
// and returns z.
void BigFloat::Neg(const Ref<BigFloat> &p_x) {
	ERR_FAIL_NULL(*p_x);

	Set(p_x);
	_neg = !_neg;
	emit_changed();
}

#define validateBinaryOperands(m_x, m_y) \
	DEV_ASSERT(!(m_x)->_mant.array.is_empty()); \
	DEV_ASSERT(!(m_y)->_mant.array.is_empty())

// z = x + y, ignoring signs of x and y for the addition
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::_uadd(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	// Note: This implementation requires 2 shifts most of the
	// time. It is also inefficient if exponents or precisions
	// differ by wide margins. The following article describes
	// an efficient (but much more complicated) implementation
	// compatible with the internal representation used here:
	//
	// Vincent Lefèvre: "The Generic Multiple-Precision Floating-
	// Point Addition With Exact Rounding (as in the MPFR Library)"
	// http://www.vinc17.net/research/papers/rnc6.pdf

	validateBinaryOperands(p_x, p_y);

	// compute exponents ex, ey for mantissa with "binary point"
	// on the right (mantissa.0) - use int64 to avoid overflow
	int64_t ex = static_cast<int64_t>(p_x->_exp) - (p_x->_mant.array.size() * 64);
	int64_t ey = static_cast<int64_t>(p_y->_exp) - (p_y->_mant.array.size() * 64);

	const bool al = this == *p_x || this == *p_y;

	// TODO(gri) having a combined add-and-shift primitive
	//           could make this code significantly faster
	if (ex < ey) {
		if (al) {
			BigNat t;
			t.lsh(p_y->_mant, static_cast<uint64_t>(ey - ex));
			_mant.add(p_x->_mant, t);
		} else {
			_mant.lsh(p_y->_mant, static_cast<uint64_t>(ey - ex));
			_mant.add(p_x->_mant, _mant);
		}
	} else if (ex > ey) {
		if (al) {
			BigNat t;
			t.lsh(p_x->_mant, static_cast<uint64_t>(ex - ey));
			_mant.add(t, p_y->_mant);
		} else {
			_mant.lsh(p_x->_mant, static_cast<uint64_t>(ex - ey));
			_mant.add(_mant, p_y->_mant);
		}
		ex = ey;
	} else {
		// ex == ey, no shift needed
		_mant.add(p_x->_mant, p_y->_mant);
	}
	// len(z.mant) > 0

	_setExpAndRound(ex + (_mant.array.size() * 64) - _mant.fnorm(), 0);
}

// z = x - y for |x| > |y|, ignoring signs of x and y for the subtraction
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::_usub(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	// This code is symmetric to uadd.
	// We have not factored the common code out because
	// eventually uadd (and usub) should be optimized
	// by special-casing, and the code will diverge.

	validateBinaryOperands(p_x, p_y);

	int64_t ex = static_cast<int64_t>(p_x->_exp) - (p_x->_mant.array.size() * 64);
	int64_t ey = static_cast<int64_t>(p_y->_exp) - (p_y->_mant.array.size() * 64);

	const bool al = this == *p_x || this == *p_y;

	if (ex < ey) {
		if (al) {
			BigNat t;
			t.lsh(p_y->_mant, static_cast<uint64_t>(ey - ex));
			_mant.sub(p_x->_mant, t);
		} else {
			_mant.lsh(p_y->_mant, static_cast<uint64_t>(ey - ex));
			_mant.sub(p_x->_mant, _mant);
		}
	} else if (ex > ey) {
		if (al) {
			BigNat t;
			t.lsh(p_x->_mant, static_cast<uint64_t>(ex - ey));
			_mant.sub(t, p_y->_mant);
		} else {
			_mant.lsh(p_x->_mant, static_cast<uint64_t>(ex - ey));
			_mant.sub(_mant, p_y->_mant);
		}
		ex = ey;
	} else {
		// ex == ey, no shift needed
		_mant.sub(p_x->_mant, p_y->_mant);
	}

	// operands may have canceled each other out
	if (_mant.array.is_empty()) {
		_acc = ACC_EXACT;
		_form = FORM_ZERO;
		_neg = false;
		return;
	}
	// len(z.mant) > 0

	_setExpAndRound(ex + (_mant.array.size() * 64) - _mant.fnorm(), 0);
}

// z = x * y, ignoring signs of x and y for the multiplication
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::_umul(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	validateBinaryOperands(p_x, p_y);

	// Note: This is doing too much work if the precision
	// of z is less than the sum of the precisions of x
	// and y which is often the case (e.g., if all floats
	// have the same precision).
	// TODO(gri) Optimize this for the common case.

	int64_t e = static_cast<int64_t>(p_x->_exp) + static_cast<int64_t>(p_y->_exp);
	if (p_x == p_y) {
		_mant.sqr(p_x->_mant);
	} else {
		_mant.mul(p_x->_mant, p_y->_mant);
	}

	_setExpAndRound(e - _mant.fnorm(), 0);
}

// z = x / y, ignoring signs of x and y for the division
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::_uquo(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	validateBinaryOperands(p_x, p_y);

	// mantissa length in words for desired result precision + 1
	// (at least one extra bit so we get the rounding bit after
	// the division)
	int64_t n = static_cast<int64_t>(_prec / 64) + 1;

	// compute adjusted x.mant such that we get enough result precision
	BigNat xadj = p_x->_mant;
	int64_t d = n - p_x->_mant.array.size() + p_y->_mant.array.size();
	if (d > 0) {
		// d extra words needed => add d "0 digits" to x
		xadj.array.resize(d);
		xadj.array.fill(0);
		xadj.array.append_array(p_x->_mant.array);
	}
	// TODO(gri): If we have too many digits (d < 0), we should be able
	// to shorten x for faster division. But we must be extra careful
	// with rounding in that case.

	// Compute d before division since there may be aliasing of x.mant
	// (via xadj) or y.mant with z.mant.
	d = xadj.array.size() - p_y->_mant.array.size();

	// divide
	BigNat r;
	_mant.div(r, xadj, p_y->_mant);
	const int64_t e = static_cast<int64_t>(p_x->_exp) - static_cast<int64_t>(p_y->_exp) - ((d - _mant.array.size()) * 64);

	// The result is long enough to include (at least) the rounding bit.
	// If there's a non-zero remainder, the corresponding fractional part
	// (if it were computed), would have a non-zero sticky bit (if it were
	// zero, it couldn't have a non-zero remainder).
	const uint64_t sbit = r.array.is_empty() ? 0 : 1;

	_setExpAndRound(e - _mant.fnorm(), sbit);
}

// ucmp returns -1, 0, or +1, depending on whether
// |x| < |y|, |x| == |y|, or |x| > |y|.
// x and y must have a non-empty mantissa and valid exponent.
int BigFloat::_ucmp(const Ref<BigFloat> &p_y) const {
	validateBinaryOperands(this, p_y);

	if (_exp < p_y->_exp) {
		return -1;
	}
	if (_exp > p_y->_exp) {
		return +1;
	}
	// x.exp == y.exp

	// compare mantissas
	int64_t i = _mant.array.size();
	int64_t j = p_y->_mant.array.size();
	while (i > 0 || j > 0) {
		BigWord xm = 0;
		BigWord ym = 0;
		if (i > 0) {
			i--;
			xm = _mant[i];
		}
		if (j > 0) {
			j--;
			ym = p_y->_mant[j];
		}

		if (xm < ym) {
			return -1;
		}
		if (xm > ym) {
			return +1;
		}
	}

	return 0;
}

// Handling of sign bit as defined by IEEE 754-2008, section 6.3:
//
// When neither the inputs nor result are NaN, the sign of a product or
// quotient is the exclusive OR of the operands’ signs; the sign of a sum,
// or of a difference x−y regarded as a sum x+(−y), differs from at most
// one of the addends’ signs; and the sign of the result of conversions,
// the quantize operation, the roundToIntegral operations, and the
// roundToIntegralExact (see 5.3.1) is the sign of the first or only operand.
// These rules shall apply even when operands or results are zero or infinite.
//
// When the sum of two operands with opposite signs (or the difference of
// two operands with like signs) is exactly zero, the sign of that sum (or
// difference) shall be +0 in all rounding-direction attributes except
// roundTowardNegative; under that attribute, the sign of an exact zero
// sum (or difference) shall be −0. However, x+x = x−(−x) retains the same
// sign as x even when x is zero.
//
// See also: https://play.golang.org/p/RtH3UCt5IH

// Add sets z to the rounded sum x+y and returns z. If z's precision is 0,
// it is changed to the larger of x's or y's precision before the operation.
// Rounding is performed according to z's precision and rounding mode; and
// z's accuracy reports the result error relative to the exact (not rounded)
// result. Add panics with [ErrNaN] if x and y are infinities with opposite
// signs. The value of z is undefined in that case.
Error BigFloat::Add(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);

#ifdef DEBUG_ENABLED
	p_x->_validate();
	p_y->_validate();
#endif

	// +Inf + -Inf
	// -Inf + +Inf
	ERR_FAIL_COND_V_MSG(p_x->_form == FORM_INF && p_y->_form == FORM_INF && p_x->_neg != p_y->_neg, ERR_INVALID_PARAMETER, "addition of infinities with opposite signs");

	if (_prec == 0) {
		_prec = Math::max(p_x->_prec, p_y->_prec);
	}

	if (p_x->_form == FORM_FINITE && p_y->_form == FORM_FINITE) {
		// x + y (common case)

		// Below we set z.neg = x.neg, and when z aliases y this will
		// change the y operand's sign. This is fine, because if an
		// operand aliases the receiver it'll be overwritten, but we still
		// want the original x.neg and y.neg values when we evaluate
		// x.neg != y.neg, so we need to save y.neg before setting z.neg.
		const bool yneg = p_y->_neg;

		_neg = p_x->_neg;
		if (p_x->_neg == yneg) {
			// x + y == x + y
			// (-x) + (-y) == -(x + y)
			_uadd(p_x, p_y);
		} else {
			// x + (-y) == x - y == -(y - x)
			// (-x) + y == y - x == -(x - y)
			if (p_x->_ucmp(p_y) > 0) {
				_usub(p_x, p_y);
			} else {
				_neg = !_neg;
				_usub(p_y, p_x); // NOLINT(readability-suspicious-call-argument)
			}
		}

		if (_form == FORM_ZERO && _mode == TO_NEGATIVE_INF && _acc == ACC_EXACT) {
			_neg = true;
		}

		emit_changed();
		return OK;
	}

	if (p_x->_form == FORM_ZERO && p_y->_form == FORM_ZERO) {
		// ±0 + ±0
		_acc = ACC_EXACT;
		_form = FORM_ZERO;
		_neg = p_x->_neg && p_y->_neg; // -0 + -0 == -0
		emit_changed();
		return OK;
	}

	if (p_x->_form == FORM_INF || p_y->_form == FORM_ZERO) {
		// ±Inf + y
		// x + ±0
		Set(p_x);
	} else {
		// ±0 + y
		// x + ±Inf
		Set(p_y);
	}

	return OK;
}

// Sub sets z to the rounded difference x-y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Sub panics with [ErrNaN] if x and y are infinities with equal
// signs. The value of z is undefined in that case.
Error BigFloat::Sub(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);

#ifdef DEBUG_ENABLED
	p_x->_validate();
	p_y->_validate();
#endif

	// +Inf - +Inf
	// -Inf - -Inf
	ERR_FAIL_COND_V_MSG(p_x->_form == FORM_INF && p_y->_form == FORM_INF && p_x->_neg == p_y->_neg, ERR_INVALID_PARAMETER, "subtraction of infinities with equal signs");

	if (_prec == 0) {
		_prec = Math::max(p_x->_prec, p_y->_prec);
	}

	if (p_x->_form == FORM_FINITE && p_y->_form == FORM_FINITE) {
		// x - y (common case)
		const bool yneg = p_y->_neg;
		_neg = p_x->_neg;
		if (p_x->_neg != yneg) {
			// x - (-y) == x + y
			// (-x) - y == -(x + y)
			_uadd(p_x, p_y);
		} else {
			// x - y == x - y == -(y - x)
			// (-x) - (-y) == y - x == -(x - y)
			if (p_x->_ucmp(p_y) > 0) {
				_usub(p_x, p_y);
			} else {
				_neg = !_neg;
				_usub(p_y, p_x); // NOLINT(readability-suspicious-call-argument)
			}
		}
		if (_form == FORM_ZERO && _mode == TO_NEGATIVE_INF && _acc == ACC_EXACT) {
			_neg = true;
		}
		emit_changed();
		return OK;
	}

	if (p_x->_form == FORM_ZERO && p_y->_form == FORM_ZERO) {
		// ±0 - ±0
		_acc = ACC_EXACT;
		_form = FORM_ZERO;
		_neg = p_x->_neg && !p_y->_neg; // -0 - +0 == -0
		emit_changed();
		return OK;
	}

	if (p_x->_form == FORM_INF || p_y->_form == FORM_ZERO) {
		// ±Inf - y
		// x - ±0
		Set(p_x);
	} else {
		// ±0 - y
		// x - ±Inf
		Neg(p_y);
	}

	return OK;
}

// Mul sets z to the rounded product x*y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Mul panics with [ErrNaN] if one operand is zero and the other
// operand an infinity. The value of z is undefined in that case.
Error BigFloat::Mul(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);

#ifdef DEBUG_ENABLED
	p_x->_validate();
	p_y->_validate();
#endif

	// ±0 * ±Inf
	// ±Inf * ±0
	ERR_FAIL_COND_V_MSG((p_x->_form == FORM_ZERO && p_y->_form == FORM_INF) || (p_x->_form == FORM_INF && p_y->_form == FORM_ZERO), ERR_INVALID_PARAMETER, "multiplication of zero with infinity");

	if (_prec == 0) {
		_prec = Math::max(p_x->_prec, p_y->_prec);
	}

	_neg = p_x->_neg != p_y->_neg;

	if (p_x->_form == FORM_FINITE && p_y->_form == FORM_FINITE) {
		// x * y (common case)
		_umul(p_x, p_y);
		emit_changed();
		return OK;
	}

	_acc = ACC_EXACT;

	if (p_x->_form == FORM_INF || p_y->_form == FORM_INF) {
		// ±Inf * y
		// x * ±Inf
		_form = FORM_INF;
	} else {
		// ±0 * y
		// x * ±0
		_form = FORM_ZERO;
	}

	emit_changed();
	return OK;
}

// Quo sets z to the rounded quotient x/y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Quo panics with [ErrNaN] if both operands are zero or infinities.
// The value of z is undefined in that case.
Error BigFloat::Quo(const Ref<BigFloat> &p_x, const Ref<BigFloat> &p_y) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(*p_y, ERR_INVALID_PARAMETER);

#ifdef DEBUG_ENABLED
	p_x->_validate();
	p_y->_validate();
#endif

	// ±0 / ±0
	ERR_FAIL_COND_V_MSG(p_x->_form == FORM_ZERO && p_y->_form == FORM_ZERO, ERR_INVALID_PARAMETER, "division of zero by zero");
	// ±Inf / ±Inf
	ERR_FAIL_COND_V_MSG(p_x->_form == FORM_INF && p_y->_form == FORM_INF, ERR_INVALID_PARAMETER, "division of infinity by infinity");

	if (_prec == 0) {
		_prec = Math::max(p_x->_prec, p_y->_prec);
	}

	_neg = p_x->_neg != p_y->_neg;

	if (p_x->_form == FORM_FINITE && p_y->_form == FORM_FINITE) {
		// x / y (common case)
		_uquo(p_x, p_y);
		emit_changed();
		return OK;
	}

	_acc = ACC_EXACT;
	if (p_x->_form == FORM_ZERO || p_y->_form == FORM_INF) {
		// ±0 / y
		// x / ±Inf
		_form = FORM_ZERO;
	} else {
		// x / ±0
		// ±Inf / y
		_form = FORM_INF;
	}

	emit_changed();
	return OK;
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y (incl. -0 == 0, -Inf == -Inf, and +Inf == +Inf);
//   - +1 if x > y.
int BigFloat::Cmp(const Ref<BigFloat> &p_y) const {
	ERR_FAIL_NULL_V(*p_y, 0);

#ifdef DEBUG_ENABLED
	_validate();
	p_y->_validate();
#endif

	const int mx = _ord();
	const int my = p_y->_ord();
	if (mx < my) {
		return -1;
	}
	if (mx > my) {
		return +1;
	}
	// mx == my

	// only if |mx| == 1 we have to compare the mantissae
	if (mx == -1) {
		return p_y->_ucmp(const_cast<BigFloat *>(this));
	}
	if (mx == +1) {
		return _ucmp(p_y);
	}

	return 0;
}

// ord classifies x and returns:
//
//	-2 if -Inf == x
//	-1 if -Inf < x < 0
//	 0 if x == 0 (signed or unsigned)
//	+1 if 0 < x < +Inf
//	+2 if x == +Inf
int BigFloat::_ord() const {
	int m = 0;
	switch (_form) {
		case FORM_FINITE:
			m = 1;
			break;
		case FORM_ZERO:
			return 0;
		case FORM_INF:
			m = 2;
			break;
	}
	if (_neg) {
		m = -m;
	}
	return m;
}
