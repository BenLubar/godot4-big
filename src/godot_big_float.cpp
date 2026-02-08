// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2014 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"
#include "godot_big_int.h"
#include "godot_big_rat.h"
#include "godot_big_naturals.h"
#include <float.h>

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
Ref<BigFloat> BigFloat::make(double x) {
	ERR_FAIL_COND_V(Math::is_nan(x), nullptr);

	Ref<BigFloat> f;
	f.instantiate();

	return f->SetFloat64(x);
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
Ref<BigFloat> BigFloat::SetPrec(uint64_t prec) {
	_acc = ACCURACY_EXACT; // optimistically assume no rounding is needed

	// special case
	if (prec == 0) {
		_prec = 0;

		if (_form == FORM_FINITE) {
			// truncate z to 0
			_acc = _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
			_form = FORM_ZERO;
		}

		return this;
	}

	// general case
	if (prec > MAX_PREC) {
		prec = MAX_PREC;
	}

	const uint32_t old = _prec;
	_prec = uint32_t(prec);
	if (_prec < old) {
		round(0);
	}

	return this;
}

// SetMode sets z's rounding mode to mode and returns an exact z.
// z remains unchanged otherwise.
// z.SetMode(z.Mode()) is a cheap way to set z's accuracy to [Exact].
Ref<BigFloat> BigFloat::SetMode(BigFloat::RoundingMode mode) {
	_mode = mode;
	_acc = ACCURACY_EXACT;

	return this;
}

// Prec returns the mantissa precision of x in bits.
// The result may be 0 for |x| == 0 and |x| == Inf.
uint64_t BigFloat::Prec() const {
	return uint64_t(_prec);
}

// MinPrec returns the minimum precision required to represent x exactly
// (i.e., the smallest prec before x.SetPrec(prec) would start rounding x).
// The result is 0 for |x| == 0 and |x| == Inf.
uint64_t BigFloat::MinPrec() const {
	if (_form != FORM_FINITE) {
		return 0;
	}

	return uint64_t(_mant.size()) * 64 - nat_trailingZeroBits(_mant);
}

// Mode returns the rounding mode of x.
BigFloat::RoundingMode BigFloat::Mode() const {
	return _mode;
}

// Acc returns the accuracy of x produced by the most recent
// operation, unless explicitly documented otherwise by that
// operation.
BigFloat::Accuracy BigFloat::Acc() const {
	return _acc;
}

// Sign returns:
//   - -1 if x < 0;
//   - 0 if x is ±0;
//   - +1 if x > 0.
int BigFloat::Sign() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
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
int64_t BigFloat::MantExp(const Ref<BigFloat> &mant) const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	int64_t exp = 0;
	if (_form == FORM_FINITE) {
		exp = int64_t(_exp);
	}

	if (mant.is_valid()) {
		mant->Copy(const_cast<BigFloat *>(this));
		if (mant->_form == FORM_FINITE) {
			mant->_exp = 0;
		}
	}

	return exp;
}

void BigFloat::setExpAndRound(int64_t exp, uint64_t sbit) {
	if (exp < MIN_EXP) {
		// underflow
		_acc = _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
		_form = FORM_ZERO;
		return;
	}

	if (exp > MAX_EXP) {
		// overflow
		_acc = _neg ? ACCURACY_BELOW : ACCURACY_ABOVE;
		_form = FORM_INF;
		return;
	}

	_form = FORM_FINITE;
	_exp = int32_t(exp);
	round(sbit);
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
Ref<BigFloat> BigFloat::SetMantExp(const Ref<BigFloat> &mant, int64_t exp) {
	ERR_FAIL_NULL_V(*mant, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
	mant->validate();
#endif

	Copy(mant);

	if (_form == FORM_FINITE) {
		setExpAndRound(int64_t(_exp) + exp, 0);
	}

	return this;
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
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
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
	return _prec <= uint32_t(_exp) || MinPrec() <= uint32_t(_exp); // not enough bits for fractional mantissa
}

#ifdef GODOT_BIG_DEBUG_FLOAT
// debugging support
void BigFloat::validate() const {
	if (_form != FORM_FINITE) {
		return;
	}

	const int64_t m = _mant.size();
	CRASH_COND_MSG(m == 0, "nonzero finite number with empty mantissa");

	constexpr uint64_t msb = 1LLU << 63;
	CRASH_COND_MSG((uint64_t(_mant[m - 1]) & msb) == 0, vformat("msb not set in last word 0x%x of %s", uint64_t(x.mant[m - 1]), Text(FORMAT_HEX_NORM, 0)));

	CRASH_COND_MSG(_prec == 0, "zero precision finite number");
}
#endif

// round rounds z according to z.mode to z.prec bits and sets z.acc accordingly.
// sbit must be 0 or 1 and summarizes any "sticky bit" information one might
// have before calling round. z's mantissa must be normalized (with the msb set)
// or empty.
//
// CAUTION: The rounding modes [ToNegativeInf], [ToPositiveInf] are affected by the
// sign of z. For correct rounding, the sign of z must be set correctly before
// calling round.
void BigFloat::round(uint64_t sbit) {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	_acc = ACCURACY_EXACT;
	if (_form != FORM_FINITE) {
		// ±0 or ±Inf => nothing left to do
		return;
	}

	// z.form == finite && len(z.mant) > 0
	// m > 0 implies z.prec > 0 (checked by validate)

	const uint32_t m = uint32_t(_mant.size()); // present mantissa length in words
	const uint32_t bits = m * 64;              // present mantissa bits; bits > 0
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
	const uint64_t r = uint64_t(bits - _prec - 1); // rounding bit position; r >= 0
	const uint64_t rbit = nat_bit(_mant, r) & 1;   // rounding bit; be safe and ensure it's a single bit
	// The sticky bit is only needed for rounding ToNearestEven
	// or when the rounding bit is zero. Avoid computation otherwise.
	if (sbit == 0 && (rbit == 0 || _mode == TO_NEAREST_EVEN)) {
		sbit = nat_sticky(_mant, r);
	}
	sbit &= 1; // be safe and ensure it's a single bit

	// cut off extra words
	const uint32_t n = (_prec + (64 - 1)) / 64; // mantissa length in words for desired precision
	if (m > n) {
		_mant = _mant.slice(m - n); // move n last words to front
	}

	// determine number of trailing zero bits (ntz) and compute lsb mask of mantissa's least-significant word
	const uint32_t ntz = n * 64 - _prec; // 0 <= ntz < _W
	const uint64_t lsb = 1LLU << ntz;

	// round if result is inexact
	if ((rbit | sbit) != 0) {
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
			inc = rbit != 0 && (sbit != 0 || (_mant[0] & lsb) != 0);
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
		_acc = inc != _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;

		if (inc) {
			// add 1 to mantissa
			if (nat_addVW(_mant, 0, _mant, 0, lsb, m) != 0) {
				// mantissa overflow => adjust exponent
				if (_exp >= MAX_EXP) {
					// exponent overflow
					_form = FORM_INF;
					return;
				}

				_exp++;
				// adjust mantissa: divide by 2 to compensate for exponent adjustment
				nat_rshVU(_mant, 0, _mant, 0, 1, m);

				// set msb == carry == 1 from the mantissa overflow above
				static constexpr uint64_t msb = 1LLU << (64 - 1);
				_mant[n - 1] |= msb;
			}
		}
	}

	// zero out trailing bits in least-significant word
	_mant[0] &= ~(lsb - 1);

#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif
}

Ref<BigFloat> BigFloat::setBits64(bool neg, uint64_t x) {
	if (_prec == 0) {
		_prec = 64;
	}

	_acc = ACCURACY_EXACT;
	_neg = neg;

	if (x == 0) {
		_form = FORM_ZERO;
		return this;
	}

	// x != 0

	_form = FORM_FINITE;
	const uint64_t s = std::countl_zero(x);
	nat_setUint64(_mant, x << s);
	_exp = int32_t(64 - s); // always fits

	if (_prec < 64) {
		round(0);
	}

	return this;
}

// SetUint64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 64 (and rounding will have
// no effect).
Ref<BigFloat> BigFloat::SetUint64(uint64_t x) {
	return setBits64(false, x);
}

// SetInt64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 64 (and rounding will have
// no effect).
Ref<BigFloat> BigFloat::SetInt64(int64_t x) {
	int64_t u = x;
	if (u < 0) {
		u = -u;
	}

	// We cannot simply call z.SetUint64(uint64(u)) and change
	// the sign afterwards because the sign affects rounding.
	return setBits64(x < 0, uint64_t(u));
}

// SetFloat64 sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to 53 (and rounding will have
// no effect). SetFloat64 panics with [ErrNaN] if x is a NaN.
Ref<BigFloat> BigFloat::SetFloat64(double x) {
	ERR_FAIL_COND_V(Math::is_nan(x), nullptr);

	if (_prec == 0) {
		_prec = 53;
	}

	_acc = ACCURACY_EXACT;
	_neg = std::signbit(x); // handle -0, -Inf correctly
	if (x == 0) {
		_form = FORM_ZERO;
		return this;
	}
	if (!Math::is_finite(x)) {
		_form = FORM_INF;
		return this;
	}

	// normalized x != 0

	_form = FORM_FINITE;
	int exp;
	double fmant = std::frexp(x, &exp); // get normalized mantissa
	nat_setUint64(_mant, (1LLU << 63) | (std::bit_cast<uint64_t>(fmant) << 11));
	_exp = int32_t(exp); // always fits
	if (_prec < 53) {
		round(0);
	}

	return this;
}

// fnorm normalizes mantissa m by shifting it to the left
// such that the msb of the most-significant word (msw) is 1.
// It returns the shift amount. It assumes that len(m) != 0.
int64_t nat_fnorm(PackedInt64Array &m) {
#ifdef GODOT_BIG_DEBUG_FLOAT
	CRASH_COND(m.is_empty() || m[m.size() - 1] == 0, "msw of mantissa is 0");
#endif

	const uint64_t s = nat_nlz(m[m.size() - 1]);
	if (s > 0) {
		[[maybe_unused]] const uint64_t c = nat_lshVU(m, 0, m, 0, s, m.size());
#ifdef GODOT_BIG_DEBUG_FLOAT
		CRASH_COND(c != 0, "nlz or lshVU incorrect");
#endif
	}

	return int64_t(s);
}

// SetInt sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the larger of x.BitLen()
// or 64 (and rounding will have no effect).
Ref<BigFloat> BigFloat::SetInt(const Ref<BigInt> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	// TODO(gri) can be more efficient if z.prec > 0
	// but small compared to the size of x, or if there
	// are many trailing 0's.
	const uint32_t bits = uint32_t(x->BitLen());
	if (_prec == 0) {
		_prec = Math::max(bits, uint32_t(64));
	}
	_acc = ACCURACY_EXACT;
	_neg = x->_neg;

	if (x->_abs.is_empty()) {
		_form = FORM_ZERO;

		return this;
	}

	// x != 0
	nat_set(_mant, x->_abs);
	nat_fnorm(_mant);
	setExpAndRound(int64_t(bits), 0);

	return this;
}

// SetRat sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the largest of a.BitLen(),
// b.BitLen(), or 64; with x = a/b.
Ref<BigFloat> BigFloat::SetRat(const Ref<BigRat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	if (x->IsInt()) {
		return SetInt(x->Num());
	}

	Ref<BigFloat> a, b;
	a.instantiate();
	b.instantiate();

	a->SetInt(x->Num());
	b->SetInt(x->Denom());

	if (_prec == 0) {
		_prec = Math::max(a->_prec, b->_prec);
	}

	return Quo(a, b);
}

// SetInf sets z to the infinite Float -Inf if signbit is
// set, or +Inf if signbit is not set, and returns z. The
// precision of z is unchanged and the result is always
// [Exact].
Ref<BigFloat> BigFloat::SetInf(bool signbit) {
	_acc = ACCURACY_EXACT;
	_form = FORM_INF;
	_neg = signbit;

	return this;
}

// Set sets z to the (possibly rounded) value of x and returns z.
// If z's precision is 0, it is changed to the precision of x
// before setting z (and rounding will have no effect).
// Rounding is performed according to z's precision and rounding
// mode; and z's accuracy reports the result error relative to the
// exact (not rounded) result.
Ref<BigFloat> BigFloat::Set(const Ref<BigFloat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
#endif

	_acc = ACCURACY_EXACT;

	if (this != *x) {
		_form = x->_form;
		_neg = x->_neg;

		if (x->_form == FORM_FINITE) {
			_exp = x->_exp;
			nat_set(_mant, x->_mant);
		}

		if (_prec == 0) {
			_prec = x->_prec;
		} else if (_prec < x->_prec) {
			round(0);
		}
	}

	return this;
}

// Copy sets z to x, with the same precision, rounding mode, and accuracy as x.
// Copy returns z. If x and z are identical, Copy is a no-op.
Ref<BigFloat> BigFloat::Copy(const Ref<BigFloat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
#endif

	if (this != *x) {
		_prec = x->_prec;
		_mode = x->_mode;
		_acc = x->_acc;
		_form = x->_form;
		_neg = x->_neg;

		if (_form == FORM_FINITE) {
			nat_set(_mant, x->_mant);
			_exp = x->_exp;
		}
	}

	return this;
}

// msb32 returns the 32 most significant bits of x.
uint32_t nat_msb32(PackedInt64Array x) {
	const int64_t i = x.size() - 1;
	if (i < 0) {
		return 0;
	}

#ifdef GODOT_BIG_DEBUG_FLOAT
	CRASH_COND_MSG((x[i] & (1LLU << (64 - 1))) == 0, "x not normalized");
#endif

	return uint32_t(uint64_t(x[i]) >> 32);
}

// msb64 returns the 64 most significant bits of x.
uint64_t nat_msb64(PackedInt64Array x) {
	const int64_t i = x.size() - 1;
	if (i < 0) {
		return 0;
	}

#ifdef GODOT_BIG_DEBUG_FLOAT
	CRASH_COND_MSG((x[i] & (1LLU << (64 - 1))) == 0, "x not normalized");
#endif

	return uint64_t(x[i]);
}

// Uint64 returns the unsigned integer resulting from truncating x
// towards zero. If 0 <= x <= [math.MaxUint64], the result is [Exact]
// if x is an integer and [Below] otherwise.
// The result is (0, [Above]) for x < 0, and ([math.MaxUint64], [Below])
// for x > [math.MaxUint64].
uint64_t BigFloat::Uint64() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
		if (_neg) {
			return 0;
		}

		// 0 < x < +Inf
		if (_exp <= 0) {
			// 0 < x < 1
			return 0;
		}

		// 1 <= x < Inf
		if (_exp <= 64) {
			// u = trunc(x) fits into a uint64
			return nat_msb64(_mant) >> (64 - uint32_t(_exp));
		}

		// x too large
		return UINT64_MAX;

	case FORM_ZERO:
		return 0;

	case FORM_INF:
		return _neg ? 0 : UINT64_MAX;
	}

	// unreachable
	ERR_FAIL_V(0);
}

BigFloat::Accuracy BigFloat::Uint64Accuracy() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
		if (_neg) {
			return ACCURACY_ABOVE;
		}

		// 0 < x < +Inf
		if (_exp <= 0) {
			// 0 < x < 1
			return ACCURACY_BELOW;
		}

		// 1 <= x < Inf
		if (_exp <= 64) {
			// u = trunc(x) fits into a uint64
			if (MinPrec() <= 64) {
				return ACCURACY_EXACT;
			}

			return ACCURACY_BELOW; // x truncated
		}

		// x too large
		return ACCURACY_BELOW;

	case FORM_ZERO:
		return ACCURACY_EXACT;

	case FORM_INF:
		return _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Int64 returns the integer resulting from truncating x towards zero.
// If [math.MinInt64] <= x <= [math.MaxInt64], the result is [Exact] if x is
// an integer, and [Above] (x < 0) or [Below] (x > 0) otherwise.
// The result is ([math.MinInt64], [Above]) for x < [math.MinInt64],
// and ([math.MaxInt64], [Below]) for x > [math.MaxInt64].
int64_t BigFloat::Int64() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
		// 0 < |x| < +Inf
		if (_exp <= 0) {
			// 0 < |x| < 1
			return 0;
		}

		// x.exp > 0

		// 1 <= |x| < +Inf
		if (_exp <= 63) {
			// i = trunc(x) fits into an int64 (excluding math.MinInt64)
			int64_t i = int64_t(nat_msb64(_mant) >> (64 - uint32_t(_exp)));
			if (_neg) {
				i = -i;
			}

			return i;
		}

		if (_neg) {
			return INT64_MIN;
		}

		// x too large
		return INT64_MAX;

	case FORM_ZERO:
		return 0;

	case FORM_INF:
		return _neg ? INT64_MIN : INT64_MAX;
	}

	// unreachable
	ERR_FAIL_V(0);
}

BigFloat::Accuracy BigFloat::Int64Accuracy() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf
		Accuracy acc = _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
		if (_exp <= 0) {
			// 0 < |x| < 1
			return acc;
		}

		// x.exp > 0

		// 1 <= |x| < +Inf
		if (_exp <= 63) {
			// i = trunc(x) fits into an int64 (excluding math.MinInt64)
			if (MinPrec() <= uint64_t(_exp)) {
				return ACCURACY_EXACT;
			}

			return acc; // x truncated
		}

		if (_neg) {
			// check for special case x == math.MinInt64 (i.e., x == -(0.5 << 64))
			if (_exp == 64 && MinPrec() == 1) {
				acc = ACCURACY_EXACT;
			}

			return acc;
		}

		// x too large
		return ACCURACY_BELOW;
	}

	case FORM_ZERO:
		return ACCURACY_EXACT;

	case FORM_INF:
		return _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Float32 returns the float32 value nearest to x. If x is too small to be
// represented by a float32 (|x| < [math.SmallestNonzeroFloat32]), the result
// is (0, [Below]) or (-0, [Above]), respectively, depending on the sign of x.
// If x is too large to be represented by a float32 (|x| > [math.MaxFloat32]),
// the result is (+Inf, [Above]) or (-Inf, [Below]), depending on the sign of x.
float BigFloat::Float32() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf

		static constexpr uint64_t fbits = 32;                         //        float size
		static constexpr uint64_t mbits = 23;                         //        mantissa size (excluding implicit msb)
		static constexpr uint64_t ebits = fbits - mbits - 1;          //     8  exponent size
		static constexpr uint64_t bias  = (1LLU << (ebits - 1)) - 1;  //   127  exponent bias
		static constexpr uint64_t dmin  = 1 - bias - mbits;           //  -149  smallest unbiased exponent (denormal)
		static constexpr uint64_t emin  = 1 - bias;                   //  -126  smallest unbiased exponent (normal)
		static constexpr uint64_t emax  = bias;                       //   127  largest unbiased exponent (normal)

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
			if (p < 0 /* m <= 0.25 */ || (p == 0 && nat_sticky(_mant, uint64_t(_mant.size()) * 64 - 1) == 0 /* m == 0.5 */)) {
				// underflow to ±0
				if (_neg) {
					const float z = 0.0;
					return -z;
				}

				return 0.0;
			}

			// otherwise, round up
			// We handle p == 0 explicitly because it's easy and because
			// Float.round doesn't support rounding to 0 bits of precision.
			if (p == 0) {
				return _neg ? -FLT_TRUE_MIN : FLT_TRUE_MIN;
			}
		}

		// p > 0

		// round
		Ref<BigFloat> r;
		r.instantiate();
		r->_prec = uint32_t(p);
		r->Set(const_cast<BigFloat *>(this));
		e = r->_exp - 1;

		// Rounding may have caused r to overflow to ±Inf
		// (rounding never causes underflows to 0).
		// If the exponent is too large, also overflow to ±Inf.
		if (r->_form == FORM_INF || e > emax) {
			// overflow
			if (_neg) {
				return -INFINITY;
			}

			return INFINITY;
		}

		// e <= emax

		// Determine sign, biased exponent, and mantissa.
		uint32_t sign = 0, bexp = 0, mant = 0;
		if (_neg) {
			sign = 1 << (fbits - 1);
		}

		// Rounding may have caused a denormal number to
		// become normal. Check again.
		if (e < emin) {
			// denormal number: recompute precision
			// Since rounding may have at best increased precision
			// and we have eliminated p <= 0 early, we know p > 0.
			// bexp == 0 for denormals
			p = mbits + 1 - emin + int32_t(e);
			mant = nat_msb32(r->_mant) >> uint32_t(fbits - p);
		} else {
			// normal number: emin <= e <= emax
			bexp = uint32_t(e + bias) << mbits;
			mant = (nat_msb32(r->_mant) >> ebits) & ((1 << mbits) - 1); // cut off msb (implicit 1 bit)
		}

		return std::bit_cast<float>(sign | bexp | mant);
	}
	case FORM_ZERO:
		if (_neg) {
			const float z = 0.0;
			return -z;
		}

		return 0.0;

	case FORM_INF:
		if (_neg) {
			return -INFINITY;
		}

		return INFINITY;
	}

	// unreachable
	ERR_FAIL_V(0.0);
}

BigFloat::Accuracy BigFloat::Float32Accuracy() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf

		static constexpr uint64_t fbits = 32;                         //        float size
		static constexpr uint64_t mbits = 23;                         //        mantissa size (excluding implicit msb)
		static constexpr uint64_t ebits = fbits - mbits - 1;          //     8  exponent size
		static constexpr uint64_t bias  = (1LLU << (ebits - 1)) - 1;  //   127  exponent bias
		static constexpr uint64_t dmin  = 1 - bias - mbits;           //  -149  smallest unbiased exponent (denormal)
		static constexpr uint64_t emin  = 1 - bias;                   //  -126  smallest unbiased exponent (normal)
		static constexpr uint64_t emax  = bias;                       //   127  largest unbiased exponent (normal)

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
			if (p < 0 /* m <= 0.25 */ || (p == 0 && nat_sticky(_mant, uint64_t(_mant.size()) * 64 - 1) == 0 /* m == 0.5 */)) {
				// underflow to ±0
				return _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
			}

			// otherwise, round up
			// We handle p == 0 explicitly because it's easy and because
			// Float.round doesn't support rounding to 0 bits of precision.
			if (p == 0) {
				return _neg ? ACCURACY_BELOW : ACCURACY_ABOVE;
			}
		}

		// p > 0

		// round
		Ref<BigFloat> r;
		r.instantiate();
		r->_prec = uint32_t(p);
		r->Set(const_cast<BigFloat *>(this));
		e = r->_exp - 1;

		// Rounding may have caused r to overflow to ±Inf
		// (rounding never causes underflows to 0).
		// If the exponent is too large, also overflow to ±Inf.
		if (r->_form == FORM_INF || e > emax) {
			// overflow
			return _neg ? ACCURACY_BELOW : ACCURACY_ABOVE;
		}

		// e <= emax

		return r->_acc;
	}
	case FORM_ZERO:
		return ACCURACY_EXACT;

	case FORM_INF:
		return ACCURACY_EXACT;
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Float64 returns the float64 value nearest to x. If x is too small to be
// represented by a float64 (|x| < [math.SmallestNonzeroFloat64]), the result
// is (0, [Below]) or (-0, [Above]), respectively, depending on the sign of x.
// If x is too large to be represented by a float64 (|x| > [math.MaxFloat64]),
// the result is (+Inf, [Above]) or (-Inf, [Below]), depending on the sign of x.
double BigFloat::Float64() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf

		static constexpr uint64_t fbits = 64;                         //        float size
		static constexpr uint64_t mbits = 52;                         //        mantissa size (excluding implicit msb)
		static constexpr uint64_t ebits = fbits - mbits - 1;          //    11  exponent size
		static constexpr uint64_t bias  = (1LLU << (ebits - 1)) - 1;  //  1023  exponent bias
		static constexpr uint64_t dmin  = 1 - bias - mbits;           // -1074  smallest unbiased exponent (denormal)
		static constexpr uint64_t emin  = 1 - bias;                   // -1022  smallest unbiased exponent (normal)
		static constexpr uint64_t emax  = bias;                       //  1023  largest unbiased exponent (normal)

		// Float mantissa m is 0.5 <= m < 1.0; compute exponent e for float64 mantissa.
		int32_t e = _exp - 1; // exponent for normal mantissa m with 1.0 <= m < 2.0

		// Compute precision p for float64 mantissa.
		// If the exponent is too small, we have a denormal number before
		// rounding and fewer than p mantissa bits of precision available
		// (the exponent remains fixed but the mantissa gets shifted right).
		int32_t p = mbits + 1; // precision of normal float
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
			if (p < 0 /* m <= 0.25 */ || (p == 0 && nat_sticky(_mant, uint64_t(_mant.size()) * 64 - 1) == 0 /* m == 0.5 */)) {
				// underflow to ±0
				if (_neg) {
					const double z = 0.0;
					return -z;
				}

				return 0.0;
			}

			// otherwise, round up
			// We handle p == 0 explicitly because it's easy and because
			// Float.round doesn't support rounding to 0 bits of precision.
			if (p == 0) {
				return _neg ? -DBL_TRUE_MIN : DBL_TRUE_MIN;
			}
		}

		// p > 0

		// round
		Ref<BigFloat> r;
		r.instantiate();
		r->_prec = uint32_t(p);
		r->Set(const_cast<BigFloat *>(this));
		e = r->_exp - 1;

		// Rounding may have caused r to overflow to ±Inf
		// (rounding never causes underflows to 0).
		// If the exponent is too large, also overflow to ±Inf.
		if (r->_form == FORM_INF || e > emax) {
			// overflow
			return _neg ? -INFINITY : INFINITY;
		}

		// e <= emax

		// Determine sign, biased exponent, and mantissa.
		uint64_t sign = 0, bexp = 0, mant = 0;
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
			mant = nat_msb64(r->_mant) >> (fbits - p);
		} else {
			// normal number: emin <= e <= emax
			bexp = uint64_t(e + bias) << mbits;
			mant = (nat_msb64(r->_mant) >> ebits) & ((1LLU << mbits) - 1); // cut off msb (implicit 1 bit)
		}

		return std::bit_cast<double>(sign | bexp | mant);
	}
	case FORM_ZERO:
		if (_neg) {
			const double z = 0.0;
			return -z;
		}

		return 0.0;

	case FORM_INF:
		return _neg ? -INFINITY : INFINITY;
	}

	// unreachable
	ERR_FAIL_V(0.0);
}

BigFloat::Accuracy BigFloat::Float64Accuracy() const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf

		static constexpr uint64_t fbits = 64;                         //        float size
		static constexpr uint64_t mbits = 52;                         //        mantissa size (excluding implicit msb)
		static constexpr uint64_t ebits = fbits - mbits - 1;          //    11  exponent size
		static constexpr uint64_t bias  = (1LLU << (ebits - 1)) - 1;  //  1023  exponent bias
		static constexpr uint64_t dmin  = 1 - bias - mbits;           // -1074  smallest unbiased exponent (denormal)
		static constexpr uint64_t emin  = 1 - bias;                   // -1022  smallest unbiased exponent (normal)
		static constexpr uint64_t emax  = bias;                       //  1023  largest unbiased exponent (normal)

		// Float mantissa m is 0.5 <= m < 1.0; compute exponent e for float64 mantissa.
		int32_t e = _exp - 1; // exponent for normal mantissa m with 1.0 <= m < 2.0

		// Compute precision p for float64 mantissa.
		// If the exponent is too small, we have a denormal number before
		// rounding and fewer than p mantissa bits of precision available
		// (the exponent remains fixed but the mantissa gets shifted right).
		int32_t p = mbits + 1; // precision of normal float
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
			if (p < 0 /* m <= 0.25 */ || (p == 0 && nat_sticky(_mant, uint64_t(_mant.size()) * 64 - 1) == 0 /* m == 0.5 */)) {
				// underflow to ±0
				return _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
			}

			// otherwise, round up
			// We handle p == 0 explicitly because it's easy and because
			// Float.round doesn't support rounding to 0 bits of precision.
			if (p == 0) {
				return _neg ? ACCURACY_BELOW : ACCURACY_ABOVE;
			}
		}

		// p > 0

		// round
		Ref<BigFloat> r;
		r.instantiate();
		r->_prec = uint32_t(p);
		r->Set(const_cast<BigFloat *>(this));
		e = r->_exp - 1;

		// Rounding may have caused r to overflow to ±Inf
		// (rounding never causes underflows to 0).
		// If the exponent is too large, also overflow to ±Inf.
		if (r->_form == FORM_INF || e > emax) {
			// overflow
			return _neg ? ACCURACY_BELOW : ACCURACY_ABOVE;
		}

		// e <= emax

		return r->_acc;
	}
	case FORM_ZERO:
		return ACCURACY_EXACT;

	case FORM_INF:
		return ACCURACY_EXACT;
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Int returns the result of truncating x towards zero;
// or nil if x is an infinity.
// The result is [Exact] if x.IsInt(); otherwise it is [Below]
// for x > 0, and [Above] for x < 0.
// If a non-nil *[Int] argument z is provided, [Int] stores
// the result in z instead of allocating a new [Int].
BigFloat::Accuracy BigFloat::Int(const Ref<BigInt> &z) const {
	ERR_FAIL_NULL_V(*z, ACCURACY_EXACT);

#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf
		Accuracy acc = _neg ? ACCURACY_ABOVE : ACCURACY_BELOW;
		if (_exp <= 0) {
			// 0 < |x| < 1
			z->SetInt64(0);
			return acc;
		}

		// x.exp > 0

		// 1 <= |x| < +Inf
		// determine minimum required precision for x
		const uint64_t allBits = uint64_t(_mant.size()) * 64;
		const uint64_t exp = uint64_t(_exp);
		if (MinPrec() <= exp) {
			acc = ACCURACY_EXACT;
		}

		// shift mantissa as needed
		z->_neg = _neg;
		if (exp > allBits) {
			nat_lsh(z->_abs, _mant, exp - allBits);
		} else if (exp < allBits) {
			nat_rsh(z->_abs, _mant, allBits - exp);
		} else {
			nat_set(z->_abs, _mant);
		}

		return acc;
	}

	case FORM_ZERO:
		z->SetInt64(0);
		return ACCURACY_EXACT;

	case FORM_INF:
		ERR_FAIL_V_MSG(_neg ? ACCURACY_ABOVE : ACCURACY_BELOW, "cannot convert Infinity to a BigInt");
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Rat returns the rational number corresponding to x;
// or nil if x is an infinity.
// The result is [Exact] if x is not an Inf.
// If a non-nil *[Rat] argument z is provided, [Rat] stores
// the result in z instead of allocating a new [Rat].
BigFloat::Accuracy BigFloat::Rat(const Ref<BigRat> &z) const {
	ERR_FAIL_NULL_V(*z, ACCURACY_EXACT);

#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
#endif

	switch (_form) {
	case FORM_FINITE:
	{
		// 0 < |x| < +Inf
		const int32_t allBits = int32_t(_mant.size()) * 64;
		// build up numerator and denominator
		z->_a->_neg = _neg;
		if (_exp > allBits) {
			nat_lsh(z->_a->_abs, _mant, uint64_t(_exp - allBits));
			nat_setUint64(z->_b->_abs, 1);
			// z already in normal form
		} else if (_exp < allBits) {
			nat_set(z->_a->_abs, _mant);
			nat_setUint64(z->_b->_abs, 1);
			nat_lsh(z->_b->_abs, z->_b->_abs, uint64_t(allBits - _exp));
			z->norm();
		} else {
			nat_set(z->_a->_abs, _mant);
			nat_setUint64(z->_b->_abs, 1);
			// z already in normal form
		}

		return ACCURACY_EXACT;
	}
	case FORM_ZERO:
		z->SetInt64(0);
		return ACCURACY_EXACT;

	case FORM_INF:
		ERR_FAIL_V_MSG(_neg ? ACCURACY_ABOVE : ACCURACY_BELOW, "cannot convert Infinity to a BigRat");
	}

	// unreachable
	ERR_FAIL_V(ACCURACY_EXACT);
}

// Abs sets z to the (possibly rounded) value |x| (the absolute value of x)
// and returns z.
Ref<BigFloat> BigFloat::Abs(const Ref<BigFloat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_neg = false;

	return this;
}

// Neg sets z to the (possibly rounded) value of x with its sign negated,
// and returns z.
Ref<BigFloat> BigFloat::Neg(const Ref<BigFloat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

	Set(x);
	_neg = !_neg;

	return this;
}

#ifdef GODOT_BIG_DEBUG_FLOAT
void BigFloat::validateBinaryOperands(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	CRASH_COND_MSG(x->_mant.is_empty(), "empty mantissa for x");

	CRASH_COND_MSG(y->_mant.is_empty(), "empty mantissa for y");
}
#endif

// z = x + y, ignoring signs of x and y for the addition
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::uadd(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	// Note: This implementation requires 2 shifts most of the
	// time. It is also inefficient if exponents or precisions
	// differ by wide margins. The following article describes
	// an efficient (but much more complicated) implementation
	// compatible with the internal representation used here:
	//
	// Vincent Lefèvre: "The Generic Multiple-Precision Floating-
	// Point Addition With Exact Rounding (as in the MPFR Library)"
	// http://www.vinc17.net/research/papers/rnc6.pdf

#ifdef GODOT_BIG_DEBUG_FLOAT
	validateBinaryOperands(x, y);
#endif

	// compute exponents ex, ey for mantissa with "binary point"
	// on the right (mantissa.0) - use int64 to avoid overflow
	int64_t ex = int64_t(x->_exp) - x->_mant.size() * 64;
	int64_t ey = int64_t(y->_exp) - y->_mant.size() * 64;

	// TODO(gri) having a combined add-and-shift primitive
	//           could make this code significantly faster
	PackedInt64Array t;
	if (ex < ey) {
		nat_lsh(t, y->_mant, uint64_t(ey - ex));
		nat_add(_mant, x->_mant, t);
	} else if (ex > ey) {
		nat_lsh(t, x->_mant, uint64_t(ex - ey));
		nat_add(_mant, t, y->_mant);
		ex = ey;
	} else {
		// ex == ey, no shift needed
		nat_add(_mant, x->_mant, y->_mant);
	}

	// len(z.mant) > 0

	setExpAndRound(ex + int64_t(_mant.size()) * 64 - nat_fnorm(_mant), 0);
}

// z = x - y for |x| > |y|, ignoring signs of x and y for the subtraction
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::usub(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	// This code is symmetric to uadd.
	// We have not factored the common code out because
	// eventually uadd (and usub) should be optimized
	// by special-casing, and the code will diverge.

#ifdef GODOT_BIG_DEBUG_FLOAT
	validateBinaryOperands(x, y);
#endif

	int64_t ex = int64_t(x->_exp) - x->_mant.size() * 64;
	int64_t ey = int64_t(y->_exp) - y->_mant.size() * 64;

	if (ex < ey) {
		PackedInt64Array t;
		nat_lsh(t, y->_mant, uint64_t(ey - ex));
		nat_sub(_mant, x->_mant, t);
	} else if (ex > ey) {
		PackedInt64Array t;
		nat_lsh(t, x->_mant, uint64_t(ex - ey));
		nat_sub(_mant, t, y->_mant);
		ex = ey;
	} else {
		// ex == ey, no shift needed
		nat_sub(_mant, x->_mant, y->_mant);
	}

	// operands may have canceled each other out
	if (_mant.size() == 0) {
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		_neg = false;
		return;
	}

	// len(z.mant) > 0

	setExpAndRound(ex + _mant.size() * 64 - nat_fnorm(_mant), 0);
}

// z = x * y, ignoring signs of x and y for the multiplication
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::umul(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validateBinaryOperands(x, y);
#endif

	// Note: This is doing too much work if the precision
	// of z is less than the sum of the precisions of x
	// and y which is often the case (e.g., if all floats
	// have the same precision).
	// TODO(gri) Optimize this for the common case.

	int64_t e = int64_t(x->_exp) + int64_t(y->_exp);
	if (x == y) {
		nat_sqr(_mant, x->_mant);
	} else {
		nat_mul(_mant, x->_mant, y->_mant);
	}

	setExpAndRound(e - nat_fnorm(_mant), 0);
}

// z = x / y, ignoring signs of x and y for the division
// but using the sign of z for rounding the result.
// x and y must have a non-empty mantissa and valid exponent.
void BigFloat::uquo(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validateBinaryOperands(x, y);
#endif

	// mantissa length in words for desired result precision + 1
	// (at least one extra bit so we get the rounding bit after
	// the division)
	const int64_t n = int64_t(_prec / 64) + 1;

	// compute adjusted x.mant such that we get enough result precision
	PackedInt64Array xadj = x->_mant;
	const int64_t d1 = n - x->_mant.size() + y->_mant.size();
	for (int64_t i = 0; i < d1; i++) {
		// d extra words needed => add d "0 digits" to x
		xadj.insert(0, 0);
	}
	// TODO(gri): If we have too many digits (d < 0), we should be able
	// to shorten x for faster division. But we must be extra careful
	// with rounding in that case.

	// Compute d before division since there may be aliasing of x.mant
	// (via xadj) or y.mant with z.mant.
	const int64_t d = xadj.size() - y->_mant.size();

	// divide
	PackedInt64Array r;
	nat_div(xadj, y->_mant, _mant, r);
	const int64_t e = int64_t(x->_exp) - int64_t(y->_exp) - (d - _mant.size()) * 64;

	// The result is long enough to include (at least) the rounding bit.
	// If there's a non-zero remainder, the corresponding fractional part
	// (if it were computed), would have a non-zero sticky bit (if it were
	// zero, it couldn't have a non-zero remainder).
	const uint64_t sbit = r.is_empty() ? 0 : 1;

	setExpAndRound(e - nat_fnorm(_mant), sbit);
}

// ucmp returns -1, 0, or +1, depending on whether
// |x| < |y|, |x| == |y|, or |x| > |y|.
// x and y must have a non-empty mantissa and valid exponent.
int BigFloat::ucmp(const Ref<BigFloat> &y) const {
#ifdef GODOT_BIG_DEBUG_FLOAT
	validateBinaryOperands(this, y);
#endif

	if (_exp < y->_exp) {
		return -1;
	}

	if (_exp > y->_exp) {
		return +1;
	}

	// x.exp == y.exp

	// compare mantissas
	int64_t i = _mant.size();
	int64_t j = y->_mant.size();
	while (i > 0 || j > 0) {
		uint64_t xm = 0, ym = 0;
		if (i > 0) {
			i--;
			xm = _mant[i];
		}
		if (j > 0) {
			j--;
			ym = y->_mant[j];
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
Ref<BigFloat> BigFloat::Add(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
	y->validate();
#endif

	if (_prec == 0) {
		_prec = Math::max(x->_prec, y->_prec);
	}

	if (x->_form == FORM_FINITE && y->_form == FORM_FINITE) {
		// x + y (common case)

		// Below we set z.neg = x.neg, and when z aliases y this will
		// change the y operand's sign. This is fine, because if an
		// operand aliases the receiver it'll be overwritten, but we still
		// want the original x.neg and y.neg values when we evaluate
		// x.neg != y.neg, so we need to save y.neg before setting z.neg.
		const bool yneg = y->_neg;

		_neg = x->_neg;
		if (x->_neg == yneg) {
			// x + y == x + y
			// (-x) + (-y) == -(x + y)
			uadd(x, y);
		} else {
			// x + (-y) == x - y == -(y - x)
			// (-x) + y == y - x == -(x - y)
			if (x->ucmp(y) > 0) {
				usub(x, y);
			} else {
				_neg = !_neg;
				usub(y, x);
			}
		}

		if (_form == FORM_ZERO && _mode == TO_NEGATIVE_INF && _acc == ACCURACY_EXACT) {
			_neg = true;
		}

		return this;
	}

	if (x->_form == FORM_INF && y->_form == FORM_INF && x->_neg != y->_neg) {
		// +Inf + -Inf
		// -Inf + +Inf
		// value of z is undefined but make sure it's valid
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		_neg = false;
		ERR_FAIL_V_MSG(nullptr, "addition of infinities with opposite signs");
	}

	if (x->_form == FORM_ZERO && y->_form == FORM_ZERO) {
		// ±0 + ±0
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		_neg = x->_neg && y->_neg; // -0 + -0 == -0
		return this;
	}

	if (x->_form == FORM_INF || y->_form == FORM_ZERO) {
		// ±Inf + y
		// x + ±0
		return Set(x);
	}

	// ±0 + y
	// x + ±Inf
	return Set(y);
}

// Sub sets z to the rounded difference x-y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Sub panics with [ErrNaN] if x and y are infinities with equal
// signs. The value of z is undefined in that case.
Ref<BigFloat> BigFloat::Sub(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
	y->validate();
#endif

	if (_prec == 0) {
		_prec = Math::max(x->_prec, y->_prec);
	}

	if (_form == FORM_FINITE && y->_form == FORM_FINITE) {
		// x - y (common case)
		const bool yneg = y->_neg;
		_neg = x->_neg;

		if (x->_neg != yneg) {
			// x - (-y) == x + y
			// (-x) - y == -(x + y)
			uadd(x, y);
		} else {
			// x - y == x - y == -(y - x)
			// (-x) - (-y) == y - x == -(x - y)
			if (x->ucmp(y) > 0) {
				usub(x, y);
			} else {
				_neg = !_neg;
				usub(y, x);
			}
		}

		if (_form == FORM_ZERO && _mode == TO_NEGATIVE_INF && _acc == ACCURACY_EXACT) {
			_neg = true;
		}

		return this;
	}

	if (x->_form == FORM_INF && y->_form == FORM_INF && x->_neg == y->_neg) {
		// +Inf - +Inf
		// -Inf - -Inf
		// value of z is undefined but make sure it's valid
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		_neg = false;
		ERR_FAIL_V_MSG(nullptr, "subtraction of infinities with equal signs");
	}

	if (x->_form == FORM_ZERO && y->_form == FORM_ZERO) {
		// ±0 - ±0
		_acc = ACCURACY_EXACT;
		_form = FORM_ZERO;
		_neg = x->_neg && !y->_neg; // -0 - +0 == -0
		return this;
	}

	if (x->_form == FORM_INF || y->_form == FORM_ZERO) {
		// ±Inf - y
		// x - ±0
		return Set(x);
	}

	// ±0 - y
	// x - ±Inf
	return Neg(y);
}

// Mul sets z to the rounded product x*y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Mul panics with [ErrNaN] if one operand is zero and the other
// operand an infinity. The value of z is undefined in that case.
Ref<BigFloat> BigFloat::Mul(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
	y->validate();
#endif

	if (_prec == 0) {
		_prec = Math::max(x->_prec, y->_prec);
	}

	_neg = x->_neg != y->_neg;

	if (x->_form == FORM_FINITE && y->_form == FORM_FINITE) {
		// x * y (common case)
		umul(x, y);
		return this;
	}

	_acc = ACCURACY_EXACT;
	if ((x->_form == FORM_ZERO && y->_form == FORM_INF) || (x->_form == FORM_INF && y->_form == FORM_ZERO)) {
		// ±0 * ±Inf
		// ±Inf * ±0
		// value of z is undefined but make sure it's valid
		_form = FORM_ZERO;
		_neg = false;
		ERR_FAIL_V_MSG(nullptr, "multiplication of zero with infinity");
	}

	if (x->_form == FORM_INF || y->_form == FORM_INF) {
		// ±Inf * y
		// x * ±Inf
		_form = FORM_INF;
		return this;
	}

	// ±0 * y
	// x * ±0
	_form = FORM_ZERO;
	return this;
}

// Quo sets z to the rounded quotient x/y and returns z.
// Precision, rounding, and accuracy reporting are as for [Float.Add].
// Quo panics with [ErrNaN] if both operands are zero or infinities.
// The value of z is undefined in that case.
Ref<BigFloat> BigFloat::Quo(const Ref<BigFloat> &x, const Ref<BigFloat> &y) {
	ERR_FAIL_NULL_V(*x, nullptr);
	ERR_FAIL_NULL_V(*y, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
	y->validate();
#endif

	if (_prec == 0) {
		_prec = Math::max(x->_prec, y->_prec);
	}

	_neg = x->_neg != y->_neg;

	if (x->_form == FORM_FINITE && y->_form == FORM_FINITE) {
		// x / y (common case)
		uquo(x, y);
		return this;
	}

	_acc = ACCURACY_EXACT;

	if (x->_form == FORM_ZERO && y->_form == FORM_ZERO) {
		// ±0 / ±0
		// value of z is undefined but make sure it's valid
		_form = FORM_ZERO;
		_neg = false;
		ERR_FAIL_V_MSG(nullptr, "division of zero by zero");
	}

	if (x->_form == FORM_INF && y->_form == FORM_INF) {
		// ±Inf / ±Inf
		// value of z is undefined but make sure it's valid
		_form = FORM_ZERO;
		_neg = false;
		ERR_FAIL_V_MSG(nullptr, "division of infinity by infinity");
	}

	if (x->_form == FORM_ZERO || y->_form == FORM_INF) {
		// ±0 / y
		// x / ±Inf
		_form = FORM_ZERO;
		return this;
	}

	// x / ±0
	// ±Inf / y
	_form = FORM_INF;
	return this;
}

// Cmp compares x and y and returns:
//   - -1 if x < y;
//   - 0 if x == y (incl. -0 == 0, -Inf == -Inf, and +Inf == +Inf);
//   - +1 if x > y.
int BigFloat::Cmp(const Ref<BigFloat> &y) const {
	ERR_FAIL_NULL_V(*y, 0);

#ifdef GODOT_BIG_DEBUG_FLOAT
	validate();
	y->validate();
#endif

	const int mx = ord();
	const int my = y->ord();

	if (mx < my) {
		return -1;
	}

	if (mx > my) {
		return +1;
	}

	// mx == my

	// only if |mx| == 1 we have to compare the mantissae
	if (mx == -1) {
		return y->ucmp(const_cast<BigFloat *>(this));
	}
	if (mx == +1) {
		return ucmp(y);
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
int BigFloat::ord() const {
	int m = 0;
	if (_form == FORM_FINITE) {
		m = 1;
	} else if (_form == FORM_ZERO) {
		return 0;
	} else if (_form == FORM_INF) {
		m = 2;
	}

	if (_neg) {
		m = -m;
	}

	return m;
}
