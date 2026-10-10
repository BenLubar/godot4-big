// This file is ported from src/math/big/intconv.go in Go 1.27.1.
// Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements int-to-string conversion functions.

#include "godot_big_debug.h"
#include "godot_big_int.h"

using namespace godot;

// Text returns the string representation of x in the given base.
// Base must be between 2 and 62, inclusive. The result uses the
// lower-case letters 'a' to 'z' for digit values 10 to 35, and
// the upper-case letters 'A' to 'Z' for digit values 36 to 61.
// No prefix (such as "0x") is added to the string. If x is a nil
// pointer it returns "<nil>".
String BigInt::String(int64_t p_base) const {
	return _abs.itoa(_neg, p_base).get_string_from_ascii();
}

// scan sets z to the integer value corresponding to the longest possible prefix
// read from r representing a signed integer number in a given conversion base.
// It returns z, the actual conversion base used, and an error, if any. In the
// error case, the value of z is undefined but the returned value is nil. The
// syntax follows the syntax of integer literals in Go.
//
// The base argument must be 0 or a value from 2 through MaxBase. If the base
// is 0, the string prefix determines the actual conversion base. A prefix of
// “0b” or “0B” selects base 2; a “0”, “0o”, or “0O” prefix selects
// base 8, and a “0x” or “0X” prefix selects base 16. Otherwise the selected
// base is 10.
Error BigInt::_scan(godot::String &s, int64_t &base) {
	// determine sign
	bool neg;
	BigNat::scanSign(s, neg);

	// determine mantissa
	BigNat abs;
	[[maybe_unused]] int64_t fcount;
	const Error err = abs.scan(s, base, false, fcount);
	BIG_ERR_FAIL_COND_V(err != OK, err);

	_neg = !abs.is_zero() && neg; // 0 has no sign
	_abs = std::move(abs);

	return OK;
}

void BigNat::scanSign(godot::String &s, bool &neg) {
	if (s.is_empty()) {
		neg = false;
		return;
	}

	const char32_t ch = s[0];
	neg = ch == U'-';
	if (neg || ch == U'+') {
		s = s.substr(1);
	}
}
