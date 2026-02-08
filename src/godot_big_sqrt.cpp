// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2017 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_float.h"
#include "godot_big_naturals.h"

// Sqrt sets z to the rounded square root of x, and returns it.
//
// If z's precision is 0, it is changed to x's precision before the
// operation. Rounding is performed according to z's precision and
// rounding mode, but z's accuracy is not computed. Specifically, the
// result of z.Acc() is undefined.
//
// The function panics if z < 0. The value of z is undefined in that
// case.
Ref<BigFloat> BigFloat::Sqrt(const Ref<BigFloat> &x) {
	ERR_FAIL_NULL_V(*x, nullptr);

#ifdef GODOT_BIG_DEBUG_FLOAT
	x->validate();
#endif

	ERR_FAIL_COND_V_MSG(x->Sign() == -1, nullptr, "square root of negative operand");

	if (_prec == 0) {
		_prec = x->_prec;
	}

	// handle ±0 and +∞
	if (x->_form != FORM_FINITE) {
		_acc = ACCURACY_EXACT;
		_form = x->_form;
		_neg = x->_neg; // IEEE754-2008 requires √±0 = ±0
		return this;
	}

	// MantExp sets the argument's precision to the receiver's, and
	// when z.prec > x.prec this will lower z.prec. Restore it after
	// the MantExp call.
	const uint32_t prec = _prec;
	int64_t b = x->MantExp(this);
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
	sqrtInverse(this);

	// re-attach halved exponent
	return SetMantExp(this, b / 2);
}

// Compute √x (to z.prec precision) by solving
//
//	1/t² - x = 0
//
// for t (using Newton's method), and then inverting.
void BigFloat::sqrtInverse(const Ref<BigFloat> &x) {
	// let
	//   f(t) = 1/t² - x
	// then
	//   g(t) = f(t)/f'(t) = -½t(1 - xt²)
	// and the next guess is given by
	//   t2 = t - g(t) = ½t(3 - xt²)
	Ref<BigFloat> u, v;
	u.instantiate();
	v.instantiate();

	const double xf = x->Float64();

	Ref<BigFloat> sqi;
	sqi.instantiate();
	sqi->SetFloat64(1 / Math::sqrt(xf));
	for (uint32_t prec = _prec + 32; sqi->_prec < prec; ) {
		sqi->_prec *= 2;

		u->_prec = sqi->_prec;
		v->_prec = sqi->_prec;
		u->Mul(sqi, sqi);       // u = t²
		u->Mul(x, u);           //   = xt²
		v->Sub(*floatThree, u); // v = 3 - xt²
		u->Mul(sqi, v);         // u = t(3 - xt²)
		u->_exp--;              //   = ½t(3 - xt²)
		sqi->Set(u);
	}

	// sqi = 1/√x

	// x/√x = √x
	Mul(x, sqi);
}
