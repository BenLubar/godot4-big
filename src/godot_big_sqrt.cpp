// This file is ported from src/math/big/sqrt.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2017 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"

using namespace godot;

extern Ref<BigFloat> *floatThree;

// Sqrt sets z to the rounded square root of x, and returns it.
//
// If z's precision is 0, it is changed to x's precision before the
// operation. Rounding is performed according to z's precision and
// rounding mode, but z's accuracy is not computed. Specifically, the
// result of z.Acc() is undefined.
//
// The function panics if z < 0. The value of z is undefined in that
// case.
Error BigFloat::Sqrt(const Ref<BigFloat> &p_x) {
	ERR_FAIL_NULL_V(*p_x, ERR_INVALID_PARAMETER);

#ifdef DBGFLAG_ASSERT
	_validate();
#endif

	if (_prec == 0) {
		_prec = p_x->_prec;
	}

	ERR_FAIL_COND_V_MSG(p_x->Sign() == -1, ERR_PARAMETER_RANGE_ERROR, "square root of negative operand");

	// handle ±0 and +∞
	if (p_x->_form != FORM_FINITE) {
		_acc = ACC_EXACT;
		_form = p_x->_form;
		_neg = p_x->_neg; // IEEE754-2008 requires √±0 = ±0
		emit_changed();
		return OK;
	}

	// MantExp sets the argument's precision to the receiver's, and
	// when z.prec > x.prec this will lower z.prec. Restore it after
	// the MantExp call.
	const uint32_t prec = _prec;
	const int32_t b = p_x->MantExp(this);
	_prec = prec;

	// Compute √(z·2**b) as
	//   √( z)·2**(½b)     if b is even
	//   √(2z)·2**(⌊½b⌋)   if b > 0 is odd
	//   √(½z)·2**(⌈½b⌉)   if b < 0 is odd
	switch (b % 2) {
		case 0:
			// nothing to do
			break;
		case 1:
			_exp++;
			break;
		case -1:
			_exp--;
			break;
	}
	// 0.25 <= z < 2.0

	// Solving 1/x² - z = 0 avoids Quo calls and is faster, especially
	// for high precisions.
	_sqrtInverse(this);

	// re-attach halved exponent
	SetMantExp(this, b / 2);

	return OK;
}

// Compute √x (to z.prec precision) by solving
//
//	1/t² - x = 0
//
// for t (using Newton's method), and then inverting.
void BigFloat::_sqrtInverse(const Ref<BigFloat> &p_x) {
	// let
	//   f(t) = 1/t² - x
	// then
	//   g(t) = f(t)/f'(t) = -½t(1 - xt²)
	// and the next guess is given by
	//   t2 = t - g(t) = ½t(3 - xt²)
	Ref<BigFloat> u{ memnew(BigFloat) };
	Ref<BigFloat> v{ memnew(BigFloat) };

	const Pair<double, BigAccuracy> xf = p_x->Float64();
	Ref<BigFloat> t{ memnew(BigFloat) };
	t->SetFloat64(1.0 / Math::sqrt(xf.first));
	for (uint32_t prec = _prec + 32; t->_prec < prec;) {
		t->_prec *= 2;
		u->_prec = t->_prec;
		v->_prec = t->_prec;
		u->Mul(t, t); // u = t²
		u->Mul(p_x, u); //   = xt²
		v->Sub(*floatThree, u); // v = 3 - xt²
		u->Mul(t, v); // u = t(3 - xt²)
		u->_exp--; //   = ½t(3 - xt²)
		t->Set(u);
	}
	// sqi = 1/√x

	// x/√x = √x
	Mul(p_x, t);
}
