// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_int.h"
#include "godot_big_naturals.h"

// This file implements int-to-string conversion functions.

// Text returns the string representation of x in the given base.
// Base must be between 2 and 62, inclusive. The result uses the
// lower-case letters 'a' to 'z' for digit values 10 to 35, and
// the upper-case letters 'A' to 'Z' for digit values 36 to 61.
// No prefix (such as "0x") is added to the string.
String BigInt::Text(int64_t base) const {
	return nat_itoa(_abs, _neg, base).get_string_from_ascii();
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
Ref<BigInt> BigInt::scan(const PackedByteArray &buf, int64_t &i, int64_t &base) {
	// determine sign
	bool neg = false;
	ERR_FAIL_COND_V(!nat_scanSign(buf, i, neg), nullptr);

	// determine mantissa
	int64_t count = 0;
	ERR_FAIL_COND_V(!nat_scan(_abs, buf, i, base, count, false), nullptr);

	_neg = !_abs.is_empty() && neg; // 0 has no sign

	return this;
}

bool nat_scanSign(const PackedByteArray &buf, int64_t &i, bool &neg) {
	if (i >= buf.size()) {
		return false;
	}

	const char ch = buf[i++];

	if (ch == '-') {
		neg = true;
	} else if (ch == '+') {
		neg = false;
	} else {
		i--;
	}

	return true;
}
