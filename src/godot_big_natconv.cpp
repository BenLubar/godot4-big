// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"
#include "godot_big_int.h"

// This file implements nat-to-string conversion functions.

static constexpr char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Note: MaxBase = len(digits), but it must remain an untyped rune constant
//       for API compatibility.

// MaxBase is the largest number base accepted for string conversions.
//const MaxBase = 10 + ('z' - 'a' + 1) + ('Z' - 'A' + 1)
static constexpr int64_t maxBaseSmall = 10 + ('z' - 'a' + 1);

// Split blocks greater than leafSize Words (or set to 0 to disable recursive conversion)
// Benchmark and configure leafSize using: go test -bench="Leaf"
//
//	8 and 16 effective on 3.0 GHz Xeon "Clovertown" CPU (128 byte cache lines)
//	8 and 16 effective on 2.66 GHz Core 2 Duo "Penryn" CPU
static constexpr int64_t leafSize = 8; // number of Word-size binary values treat as a monolithic block

// maxPow returns (b**n, n) such that b**n is the largest power b**n <= _M.
// For instance maxPow(10) == (1e19, 19) for 19 decimal digits in a 64bit Word.
// In other words, at most n digits in base b fit into a Word.
// TODO(gri) replace this with a table, generated at build time.
void nat_maxPow(uint64_t b, uint64_t &p, int64_t &n) {
	p = b, n = 1; // assuming b <= _M
	for (int64_t max = (~uint64_t(0)) / b; p <= max; ) {
		// p == b**n && p <= max
		p *= b;
		n++;
	}

	// p == b**n && p <= _M
}

// pow returns x**n for n > 0, and 1 otherwise.
uint64_t nat_pow(uint64_t x, int64_t n) {
	// n == sum of bi * 2**i, for 0 <= i < imax, and bi is 0 or 1
	// thus x**n == product of x**(2**i) for all i where bi == 1
	// (Russian Peasant Method for exponentiation)
	uint64_t p = 1;

	while (n > 0) {
		if ((n & 1) != 0) {
			p *= x;
		}

		x *= x;
		n >>= 1;
	}

	return p;
}

// scan scans the number corresponding to the longest possible prefix
// from r representing an unsigned number in a given conversion base.
// scan returns the corresponding natural number res, the actual base b,
// a digit count, and a read or syntax error err, if any.
//
// For base 0, an underscore character “_” may appear between a base
// prefix and an adjacent digit, and between successive digits; such
// underscores do not change the value of the number, or the returned
// digit count. Incorrect placement of underscores is reported as an
// error if there are no other errors. If base != 0, underscores are
// not recognized and thus terminate scanning like any other character
// that is not a valid radix point or digit.
//
//	number    = mantissa | prefix pmantissa .
//	prefix    = "0" [ "b" | "B" | "o" | "O" | "x" | "X" ] .
//	mantissa  = digits "." [ digits ] | digits | "." digits .
//	pmantissa = [ "_" ] digits "." [ digits ] | [ "_" ] digits | "." digits .
//	digits    = digit { [ "_" ] digit } .
//	digit     = "0" ... "9" | "a" ... "z" | "A" ... "Z" .
//
// Unless fracOk is set, the base argument must be 0 or a value between
// 2 and MaxBase. If fracOk is set, the base argument must be one of
// 0, 2, 8, 10, or 16. Providing an invalid base argument leads to a run-
// time panic.
//
// For base 0, the number prefix determines the actual base: A prefix of
// “0b” or “0B” selects base 2, “0o” or “0O” selects base 8, and
// “0x” or “0X” selects base 16. If fracOk is false, a “0” prefix
// (immediately followed by digits) selects base 8 as well. Otherwise,
// the selected base is 10 and no prefix is accepted.
//
// If fracOk is set, a period followed by a fractional part is permitted.
// The result value is computed as if there were no period present; and
// the count value is used to determine the fractional part.
//
// For bases <= 36, lower and upper case letters are considered the same:
// The letters 'a' to 'z' and 'A' to 'Z' represent digit values 10 to 35.
// For bases > 36, the upper case letters 'A' to 'Z' represent the digit
// values 36 to 61.
//
// A result digit count > 0 corresponds to the number of (non-prefix) digits
// parsed. A digit count <= 0 indicates the presence of a period (if fracOk
// is set, only), and -count is the number of fractional digits found.
// In this case, the actual value of the scanned number is res * b**count.
bool nat_scan(PackedInt64Array &z, const PackedByteArray &buf, int64_t &i, int64_t &base, int64_t &count, bool fracOk) {
	// Reject invalid bases.
	ERR_FAIL_COND_V_MSG((base != 0 && (fracOk ? (base != 2 && base != 8 && base != 10 && base != 16) : (2 > base || base > BigInt::MAX_BASE))), false, vformat("invalid number base %d", base));

	const int64_t orig_base = base;

	// prev encodes the previously seen char: it is one
	// of '_', '0' (a digit), or '.' (anything else). A
	// valid separator '_' may only occur after a digit
	// and if base == 0.
	char prev = '.';
	bool invalSep = false;

	// one char look-ahead
	if (i >= buf.size()) {
		if (base == 0) {
			base = 10;
		}

		count = 0;

		return false;
	}

	// Determine actual base.
	char prefix = 0;
	if (base == 0) {
		// Actual base is 10 unless there's a base prefix.
		base = 10;
		if (buf[i] == '0') {
			prev = '0';
			count = 1;

			i++;
			if (i < buf.size()) {
				// possibly one of 0b, 0B, 0o, 0O, 0x, 0X
				switch (buf[i]) {
				case 'b':
				case 'B':
					base = 2;
					prefix = 'b';
					break;
				case 'o':
				case 'O':
					base = 8;
					prefix = 'o';
					break;
				case 'x':
				case 'X':
					base = 16;
					prefix = 'x';
					break;
				default:
					if (!fracOk) {
						base = 8;
						prefix = '0';
					}
					break;
				}

				if (prefix != 0) {
					count = 0; // prefix is not counted
					if (prefix != '0') {
						i++;
					}
				}
			}
		}
	}

	// Convert string.
	// Algorithm: Collect digits in groups of at most n digits in di.
	// For bases that pack exactly into words (2, 4, 16), append di's
	// directly to the int representation and then reverse at the end (bn==0 marks this case).
	// For other bases, use mulAddWW for every such group to shift
	// z up one group and add di to the result.
	// With more cleverness we could also handle binary bases like 8 and 32
	// (corresponding to 3-bit and 5-bit chunks) that don't pack nicely into
	// words, but those are not too important.
	z.clear();
	uint64_t b1 = base;
	uint64_t bn = 0; // b1**n (or 0 for the special bit-packing cases b=2,4,16)
	int64_t n = 0;   // max digits that fit into Word
	switch (base) {
	case 2: // 1 bit per digit
		n = 64;
		break;
	case 4: // 2 bits per digit
		n = 64 / 2;
		break;
	case 16: // 4 bits per digit
		n = 64 / 4;
		break;
	default:
		nat_maxPow(b1, bn, n);
		break;
	}

	uint64_t di = 0; // 0 <= di < b1**i < bn
	int64_t j = 0;   // 0 <= i < n
	int64_t dp = -1; // position of decimal point
	while (i < buf.size()) {
		char ch = buf[i];

		if (ch == '.' && fracOk) {
			fracOk = false;
			if (prev == '_') {
				invalSep = true;
			}
			prev = '.';
			dp = count;
		} else if (ch == '_' && orig_base == 0) {
			if (prev != '0') {
				invalSep = true;
			}
			prev = '_';
		} else {
			// convert rune into digit value d1
			uint64_t d1 = 0;
			if ('0' <= ch && ch <= '9') {
				d1 = uint64_t(ch - '0');
			} else if ('a' <= ch && ch <= 'z') {
				d1 = uint64_t(ch - 'a' + 10);
			} else if ('A' <= ch && ch <= 'Z') {
				if (base <= maxBaseSmall) {
					d1 = uint64_t(ch - 'A' + 10);
				} else {
					d1 = uint64_t(ch - 'A' + maxBaseSmall);
				}
			} else {
				d1 = BigInt::MAX_BASE + 1;
			}

			if (d1 >= b1) {
				i--; // ch does not belong to number anymore
				break;
			}

			prev = '0';
			count++;

			// collect d1 in di
			di = di * b1 + d1;
			j++;

			// if di is "full", add it to the result
			if (j == n) {
				if (bn == 0) {
					z.append(di);
				} else {
					nat_mulAddWW(z, z, bn, di);
				}

				di = 0;
				j = 0;
			}
		}

		i++;
	}

	// other errors take precedence over invalid separators
	ERR_FAIL_COND_V(invalSep || prev == '_', false);

	if (count == 0) {
		// no digits found
		if (prefix == '0') {
			// there was only the octal prefix 0 (possibly followed by separators and digits > 7);
			// interpret as decimal 0
			z.clear();
			base = 10;
			count = 1;
			return true;
		}

		return false;
	}

	if (bn == 0) {
		if (j > 0) {
			// Add remaining digit chunk to result.
			// Left-justify group's digits; will shift back down after reverse.
			z.append(di * nat_pow(b1, n - i));
		}
		z.reverse();
		nat_norm(z);
		if (j > 0) {
			nat_rsh(z, z, uint64_t(n - j) * uint64_t(64 / n));
		}
	} else {
		if (j > 0) {
			// Add remaining digit chunk to result.
			nat_mulAddWW(z, z, nat_pow(b1, j), di);
		}
	}

	// adjust count for fraction, if any
	if (dp >= 0) {
		// 0 <= dp <= count
		count = dp - count;
	}

	return true;
}

// utoa converts x to an ASCII representation in the given base;
// base must be between 2 and MaxBase, inclusive.
PackedByteArray nat_utoa(PackedInt64Array x, int64_t base) {
	return nat_itoa(x, false, base);
}

// itoa is like utoa but it prepends a '-' if neg && x != 0.
PackedByteArray nat_itoa(PackedInt64Array x, bool neg, int64_t base) {
	ERR_FAIL_COND_V_MSG(base < 2 || base > BigInt::MAX_BASE, PackedByteArray(), "invalid base");

	// x == 0
	if (x.is_empty()) {
		return PackedByteArray{'0'};
	}

	// len(x) > 0

	// allocate buffer for conversion
	int64_t i = int64_t(double(nat_bitLen(x)) / std::log2(double(base))) + 1; // off by 1 at most
	if (neg) {
		i++;
	}

	PackedByteArray s;
	s.resize(i);

	// convert power of two and non power of two bases separately
	const uint64_t b = uint64_t(base);
	if (b == (b & -b)) {
		// shift is base b digit size in bits
		const uint64_t shift = uint64_t(std::countr_zero(b)); // shift > 0 because b >= 2
		const uint64_t mask = (1LLU << shift) - 1;
		uint64_t w = x[0];   // current word
		uint64_t nbits = 64; // number of unprocessed bits in w

		// convert less-significant words (include leading zeros)
		for (int64_t k = 1; k < x.size(); k++) {
			// convert full digits
			while (nbits >= shift) {
				s[--i] = digits[w & mask];
				w >>= shift;
				nbits -= shift;
			}

			// convert any partial leading digit and advance to next word
			if (nbits == 0) {
				// no partial digit remaining, just advance
				w = x[k];
				nbits = 64;
			} else {
				// partial digit in current word w (== x[k-1]) and next word x[k]
				w |= uint64_t(x[k]) << nbits;
				s[--i] = digits[w & mask];

				// advance
				w = uint64_t(x[k]) >> (shift - nbits);
				nbits = 64 - (shift - nbits);
			}
		}

		// convert digits of most-significant word w (omit leading zeros)
		while (w != 0) {
			s[--i] = digits[w & mask];
			w >>= shift;
		}
	} else {
		uint64_t bb;
		int64_t ndigits;
		nat_maxPow(b, bb, ndigits);

		// construct table of successive squares of bb*leafSize to use in subdivisions
		// result (table != nil) <=> (len(x) > leafSize > 0)
		Vector<BigDivisor> table = nat_divisors(x.size(), b, ndigits, bb);

		// convert q to string s in base b
		nat_convertWords(x, s, 0, s.size(), b, ndigits, bb, table, table.size());

		// strip leading zeros
		// (x != 0; thus s must contain at least one non-zero digit
		// and the loop will terminate)
		i = 0;
		while (s[i] == '0') {
			i++;
		}
	}

	if (neg) {
		i--;
		s[i] = '-';
	}

	return s.slice(i);
}

// Convert words of q to base b digits in s. If q is large, it is recursively "split in half"
// by nat/nat division using tabulated divisors. Otherwise, it is converted iteratively using
// repeated nat/Word division.
//
// The iterative method processes n Words by n divW() calls, each of which visits every Word in the
// incrementally shortened q for a total of n + (n-1) + (n-2) ... + 2 + 1, or n(n+1)/2 divW()'s.
// Recursive conversion divides q by its approximate square root, yielding two parts, each half
// the size of q. Using the iterative method on both halves means 2 * (n/2)(n/2 + 1)/2 divW()'s
// plus the expensive long div(). Asymptotically, the ratio is favorable at 1/2 the divW()'s, and
// is made better by splitting the subblocks recursively. Best is to split blocks until one more
// split would take longer (because of the nat/nat div()) than the twice as many divW()'s of the
// iterative approach. This threshold is represented by leafSize. Benchmarking of leafSize in the
// range 2..64 shows that values of 8 and 16 work well, with a 4x speedup at medium lengths and
// ~30x for 20000 digits. Use nat_test.go's BenchmarkLeafSize tests to optimize leafSize for
// specific hardware.
void nat_convertWords(PackedInt64Array q, PackedByteArray &s, int64_t soff, int64_t n, uint64_t b, int64_t ndigits, uint64_t bb, const Vector<BigDivisor> &table, int64_t tablesize) {
	// split larger blocks recursively
	if (!table.is_empty()) {
		// len(q) > leafSize > 0
		PackedInt64Array r;
		int64_t index = tablesize - 1;
		while (q.size() > leafSize) {
			// find divisor close to sqrt(q) if possible, but in any case < q
			uint64_t maxLength = nat_bitLen(q);  // ~= log2 q, or at of least largest possible q of this bit length
			uint64_t minLength = maxLength >> 1; // ~= log2 sqrt(q)

			while (index > 0 && table[index-1].nbits > minLength) {
				index--; // desired
			}

			if (table[index].nbits >= maxLength && nat_cmp(table[index].bbb, q) >= 0) {
				index--;
				CRASH_COND(index < 0);
			}

			// split q into the two digit number (q'*bbb + r) to form independent subblocks
			nat_div(q, table[index].bbb, q, r);

			// convert subblocks and collect results in s[:h] and s[h:]
			const int64_t h = n - table[index].ndigits;
			nat_convertWords(r, s, soff + h, n - h, b, ndigits, bb, table, index);
			n = h; // == q.convertWords(stk, s, b, ndigits, bb, table[0:index+1])
		}
	}

	// having split any large blocks now process the remaining (small) block iteratively
	int64_t i = soff + n;
	uint64_t r;
	if (b == 10) {
		// hard-coding for 10 here speeds this up by 1.25x (allows for / and % by constants)
		while (!q.is_empty()) {
			// extract least significant, base bb "digit"
			r = nat_divW(q, bb, q);
			for (int64_t j = 0; j < ndigits && i > soff; j++) {
				// avoid % computation since r%10 == r - int(r/10)*10;
				// this appears to be faster for BenchmarkString10000Base10
				// and smaller strings (but a bit slower for larger ones)
				uint64_t t = r / 10;
				s[--i] = '0' + (r - t * 10);
				r = t;
			}
		}
	} else {
		while (!q.is_empty()) {
			// extract least significant, base bb "digit"
			r = nat_divW(q, bb, q);
			for (int64_t j = 0; j < ndigits && i > soff; j++) {
				s[--i] = digits[r % b];
				r /= b;
			}
		}
	}

	// prepend high-order zeros
	while (i > soff) { // while need more leading zeros
		s[--i] = '0';
	}
}

// expWW computes x**y
void nat_expWW(PackedInt64Array &z, uint64_t x, uint64_t y) {
	PackedInt64Array nx, ny;
	nat_setWord(nx, x);
	nat_setWord(ny, y);
	nat_expNN(z, nx, ny, PackedInt64Array(), false);
}

// construct table of powers of bb*leafSize to use in subdivisions.
Vector<BigDivisor> nat_divisors(int64_t m, uint64_t b, int64_t ndigits, uint64_t bb) {
	// only compute table when recursive conversion is enabled and x is large
	if (leafSize == 0 || m <= leafSize) {
		return Vector<BigDivisor>();
	}

	// determine k where (bb**leafSize)**(2**k) >= sqrt(x)
	int64_t k = 1;
	for (int64_t words = leafSize; words < (m >> 1) && k < 64; words <<= 1) {
		k++;
	}

	// reuse and extend existing table of divisors or create new table as appropriate
	Vector<BigDivisor> temp_table;
	Vector<BigDivisor> *ptable = &temp_table; // for b == 10, table overlaps with cacheBase10.table
	if (b == 10) {
		cacheBase10Mutex.lock();
		ptable = cacheBase10; // reuse old table for this conversion
		if (cacheBase10->size() < k) {
			cacheBase10->resize(k);
		}
	} else {
		temp_table.resize(k); // create new table for this conversion
	}

	Vector<BigDivisor> &table = *ptable;

	// extend table
	if (table[k - 1].ndigits == 0) {
		// add new entries as needed
		PackedInt64Array larger;
		for (int64_t i = 0; i < k; i++) {
			if (table[i].ndigits == 0) {
				if (i == 0) {
					nat_expWW(table.write[0].bbb, bb, uint64_t(leafSize));
					table.write[0].ndigits = ndigits * leafSize;
				} else {
					nat_sqr(table.write[i].bbb, table[i - 1].bbb);
					table.write[i].ndigits = 2 * table[i - 1].ndigits;
				}

				// optimization: exploit aggregated extra bits in macro blocks
				nat_set(larger, table[i].bbb);
				while (nat_mulAddVWW(larger, 0, larger, 0, b, 0, larger.size()) == 0) {
					nat_set(table.write[i].bbb, larger);
					table.write[i].ndigits++;
				}

				table.write[i].nbits = nat_bitLen(table[i].bbb);
			}
		}
	}

	// out of an abundance of caution, reference the vector while we still potentially have the lock
	temp_table = table;

	if (b == 10) {
		cacheBase10Mutex.unlock();
	}

	return temp_table;
}
