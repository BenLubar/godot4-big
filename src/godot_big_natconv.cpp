// This file is ported from src/math/big/natconv.go in Go 1.27.1.
// Original copyright notice follows:

// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements nat-to-string conversion functions.

#include "godot_big_debug.h"
#include "godot_big_naturals.h"

#include <mutex>

using namespace godot;

extern std::array<BigDivisor, 64> *cacheBase10;
extern std::mutex cacheBase10_mutex;

static constexpr const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr int64_t maxBaseSmall = 10 + ('z' - 'a' + 1);

static constexpr std::array<std::tuple<BigWord, int64_t>, BigNat::MAX_BASE> genMaxPow() {
	std::array<std::tuple<BigWord, int64_t>, BigNat::MAX_BASE> array;

	for (BigWord b = 2; b < array.size(); b++) {
		BigWord p = b;
		int64_t n = 1; // assuming b <= UINT64_MAX
		const BigWord max = UINT64_MAX / b;
		while (p <= max) {
			// p == b**n && p <= max
			p *= b;
			n++;
		}
		// p == b**n && p <= _M
		array.at(b) = std::make_tuple(p, n);
	}

	return array;
}

// maxPow returns (b**n, n) such that b**n is the largest power b**n <= _M.
// For instance maxPow(10) == (1e19, 19) for 19 decimal digits in a 64bit Word.
// In other words, at most n digits in base b fit into a Word.
// TODO(gri) replace this with a table, generated at build time.
static constexpr std::array<std::tuple<BigWord, int64_t>, BigNat::MAX_BASE> maxPow{ genMaxPow() };

// pow returns x**n for n > 0, and 1 otherwise.
BigWord BigNat::pow(BigWord x, int64_t n) {
	// n == sum of bi * 2**i, for 0 <= i < imax, and bi is 0 or 1
	// thus x**n == product of x**(2**i) for all i where bi == 1
	// (Russian Peasant Method for exponentiation)
	BigWord p = 1;
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
Error BigNat::scan(godot::String &s, int64_t &base, bool fracOk, int64_t &fcount) {
	const int64_t orig_base = base;
	// Reject invalid bases.
	bool baseOk = orig_base == 0 ||
			(!fracOk && 2 <= orig_base && orig_base <= MAX_BASE) ||
			(fracOk && (orig_base == 2 || orig_base == 8 || orig_base == 10 || orig_base == 16));
	ERR_FAIL_COND_V_MSG(!baseOk, ERR_INVALID_PARAMETER, "invalid number base " + String::num_int64(orig_base));

	// prev encodes the previously seen char: it is one
	// of '_', '0' (a digit), or '.' (anything else). A
	// valid separator '_' may only occur after a digit
	// and if base == 0.
	char prev = '.';
	bool invalSep = false;

	// one char look-ahead
	int64_t si = 0;

	// Determine actual base.
	char prefix = 0;
	if (orig_base == 0) {
		// Actual base is 10 unless there's a base prefix.
		base = 10;
		if (si < s.length() && s[si] == U'0') {
			prev = '0';
			fcount = 1;
			si++;
			if (si < s.length()) {
				// possibly one of 0b, 0B, 0o, 0O, 0x, 0X
				switch (s[si]) {
					case U'b':
					case U'B':
						base = 2;
						prefix = 'b';
						break;
					case U'o':
					case U'O':
						base = 8;
						prefix = 'o';
						break;
					case U'x':
					case U'X':
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
					fcount = 0; // prefix is not counted
					if (prefix != '0') {
						si++;
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
	BigNat z;
	BigWord b1 = static_cast<BigWord>(base);
	BigWord bn = 0; // b1**n (or 0 for the special bit-packing cases b=2,4,16)
	int64_t n; // max digits that fit into Word
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
			std::tie(bn, n) = maxPow.at(b1);
			break;
	}

	BigWord di = 0; // 0 <= di < b1**i < bn
	int64_t i = 0; // 0 <= i < n
	int64_t dp = -1; // position of decimal point
	while (si < s.length()) {
		const char32_t ch = s[si];
		if (ch == U'.' && fracOk) {
			fracOk = false;
			if (prev == '_') {
				invalSep = true;
			}
			prev = '.';
			dp = fcount;
		} else if (ch == U'_' && orig_base == 0) {
			if (prev != '0') {
				invalSep = true;
			}
			prev = '_';
		} else {
			// convert rune into digit value d1
			BigWord d1 = MAX_BASE + 1;
			if (U'0' <= ch && ch <= U'9') {
				d1 = static_cast<BigWord>(ch - U'0');
			} else if (U'a' <= ch && ch <= U'z') {
				d1 = static_cast<BigWord>(ch - U'a' + 10); // NOLINT(bugprone-misplaced-widening-cast)
			} else if (U'A' <= ch && ch <= U'Z') {
				if (base <= maxBaseSmall) {
					d1 = static_cast<BigWord>(ch - U'A' + 10); // NOLINT(bugprone-misplaced-widening-cast)
				} else {
					d1 = static_cast<BigWord>(ch - U'A' + maxBaseSmall);
				}
			}
			if (d1 >= b1) {
				break; // ch does not belong to number anymore
			}
			prev = '0';
			fcount++;

			// collect d1 in di
			di = (di * b1) + d1;
			i++;

			// if di is "full", add it to the result
			if (i == n) {
				if (bn == 0) {
					z.array.append(static_cast<int64_t>(di));
				} else {
					z.mulAddWW(z, bn, di);
				}
				di = 0;
				i = 0;
			}
		}

		si++;
	}

	ERR_FAIL_COND_V_MSG(invalSep || prev == '_', ERR_INVALID_DATA, "'_' must separate successive digits");

	s = s.substr(si);

	if (fcount == 0) {
		// no digits found
		if (prefix == '0') {
			// there was only the octal prefix 0 (possibly followed by separators and digits > 7);
			// interpret as decimal 0
			array.clear();
			base = 10;
			fcount = 1;
			return OK;
		}

		ERR_FAIL_V_MSG(ERR_INVALID_DATA, "number has no digits");
	}

	if (bn == 0) {
		if (i > 0) {
			// Add remaining digit chunk to result.
			// Left-justify group's digits; will shift back down after reverse.
			z.array.append(static_cast<int64_t>(di * pow(b1, n - i)));
		}
		z.array.reverse();
		z.norm();
		if (i > 0) {
			z.rsh(z, static_cast<uint64_t>(n - i) * static_cast<uint64_t>(64 / n));
		}
	} else if (i > 0) {
		// Add remaining digit chunk to result.
		z.mulAddWW(z, pow(b1, i), di);
	}
	*this = z;

	// adjust count for fraction, if any
	if (dp >= 0) {
		// 0 <= dp <= count
		fcount = dp - fcount;
	}

	return OK;
}

// utoa converts x to an ASCII representation in the given base;
// base must be between 2 and MaxBase, inclusive.
PackedByteArray BigNat::utoa(int64_t p_base) const {
	return itoa(false, p_base);
}

// itoa is like utoa but it prepends a '-' if neg && x != 0.
PackedByteArray BigNat::itoa(bool p_neg, int64_t p_base) const {
	ERR_FAIL_COND_V_MSG(p_base < 2 || p_base > MAX_BASE, {}, "invalid base");

	// x == 0
	if (is_zero()) {
		return { '0' };
	}
	// len(x) > 0

	// allocate buffer for conversion
	int64_t i = static_cast<int64_t>(static_cast<double>(bitLen()) / std::log2(static_cast<double>(p_base))) + 1; // off by 1 at most
	if (p_neg) {
		i++;
	}

	PackedByteArray s;
	s.resize(i);

	// convert power of two and non power of two bases separately
	BigWord b = static_cast<BigWord>(p_base);
	if (b == (b & -b)) {
		// shift is base b digit size in bits
		uint64_t shift = static_cast<uint64_t>(std::countr_zero(b)); // shift > 0 because b >= 2
		BigWord mask = (UINT64_C(1) << shift) - 1;
		BigWord w = operator[](0); // current word
		uint64_t nbits = 64; // number of unprocessed bits in w

		// convert less-significant words (include leading zeros)
		for (int64_t k = 1; k < array.size(); k++) {
			// convert full digits
			while (nbits >= shift) {
				i--;
				s[i] = digits[w & mask];
				w >>= shift;
				nbits -= shift;
			}

			// convert any partial leading digit and advance to next word
			if (nbits == 0) {
				// no partial digit remaining, just advance
				w = operator[](k);
				nbits = 64;
			} else {
				// partial digit in current word w (== x[k-1]) and next word x[k]
				w |= operator[](k) << nbits;
				i--;
				s[i] = digits[w & mask];

				// advance
				w = operator[](k) >> (shift - nbits);
				nbits = 64 - (shift - nbits);
			}
		}

		// convert digits of most-significant word w (omit leading zeros)
		while (w != 0) {
			i--;
			s[i] = digits[w & mask];
			w >>= shift;
		}
	} else {
		BigWord bb;
		int64_t ndigits;
		std::tie(bb, ndigits) = maxPow.at(b);

		// construct table of successive squares of bb*leafSize to use in subdivisions
		// result (table != nil) <=> (len(x) > leafSize > 0)
		LocalVector<BigDivisor> table;
		divisors(array.size(), b, ndigits, bb, table);

		// preserve x, create local copy for use by convertWords
		BigNat q{ *this };

		// convert q to string s in base b
		q.convertWords(s.ptrw(), s.size(), b, ndigits, bb, table.span());

		// strip leading zeros
		// (x != 0; thus s must contain at least one non-zero digit
		// and the loop will terminate)
		i = 0;
		while (s[i] == '0') {
			i++;
		}
	}

	if (p_neg) {
		i--;
		s[i] = '-';
	}

	return s.slice(i);
}

// Split blocks greater than leafSize Words (or set to 0 to disable recursive conversion)
// Benchmark and configure leafSize using: go test -bench="Leaf"
//
//	8 and 16 effective on 3.0 GHz Xeon "Clovertown" CPU (128 byte cache lines)
//	8 and 16 effective on 2.66 GHz Core 2 Duo "Penryn" CPU
CALIBRATION_SPECIFIERS int64_t leafSize = 8; // number of Word-size binary values treat as a monolithic block

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
void BigNat::convertWords(uint8_t *s, int64_t slen, BigWord p_b, int64_t p_ndigits, BigWord p_bb, const Span<BigDivisor> &p_table) {
	// split larger blocks recursively
	if (!p_table.is_empty()) {
		// len(q) > leafSize > 0
		BigNat r;
		uint32_t index = p_table.size() - 1;
		while (array.size() > leafSize) {
			// find divisor close to sqrt(q) if possible, but in any case < q
			int64_t maxLength = bitLen(); // ~= log2 q, or at of least largest possible q of this bit length
			int64_t minLength = maxLength >> 1; // ~= log2 sqrt(q)
			while (index > 0 && p_table[index - 1].nbits > minLength) {
				index--; // desired
			}
			if (p_table[index].nbits >= maxLength && p_table[index].bbb.cmp(*this) >= 0) {
				index--;
				CRASH_COND(index < 0);
			}

			// split q into the two digit number (q'*bbb + r) to form independent subblocks
			div(*this, p_table[index].bbb, r);

			// convert subblocks and collect results in s[:h] and s[h:]
			int64_t h = slen - p_table[index].ndigits;
			r.convertWords(s + h, slen - h, p_b, p_ndigits, p_bb, { p_table.ptr(), index });
			slen = h; // == q.convertWords(stk, s, b, ndigits, bb, table[0:index+1])
		}
	}

	// having split any large blocks now process the remaining (small) block iteratively
	int64_t i = slen;
	BigWord r = 0;
	if (p_b == 10) {
		// hard-coding for 10 here speeds this up by 1.25x (allows for / and % by constants)
		while (!is_zero()) {
			// extract least significant, base bb "digit"
			r = divW(*this, p_bb);
			for (int64_t j = 0; j < p_ndigits && i > 0; j++) {
				i--;
				// avoid % computation since r%10 == r - int(r/10)*10;
				// this appears to be faster for BenchmarkString10000Base10
				// and smaller strings (but a bit slower for larger ones)
				const BigWord t = r / 10;
				s[i] = '0' + static_cast<uint8_t>(r - (t * 10));
				r = t;
			}
		}
	} else {
		while (!is_zero()) {
			// extract least significant, base bb "digit"
			r = divW(*this, p_bb);
			for (int64_t j = 0; j < p_ndigits && i > 0; j++) {
				i--;
				s[i] = digits[r % p_b];
				r /= p_b;
			}
		}
	}

	// prepend high-order zeros
	while (i > 0) { // while need more leading zeros
		i--;
		s[i] = '0';
	}
}

// expWW computes x**y
void BigNat::expWW(BigWord p_x, BigWord p_y) {
	expNN({ p_x }, { p_y }, {}, false);
}

// construct table of powers of bb*leafSize to use in subdivisions.
void BigNat::divisors(int64_t p_m, BigWord p_b, int64_t p_ndigits, BigWord p_bb, LocalVector<BigDivisor> &r_table) {
	// only compute table when recursive conversion is enabled and x is large
	if (leafSize == 0 || p_m <= leafSize) {
		return;
	}

	// determine k where (bb**leafSize)**(2**k) >= sqrt(x)
	uint32_t k = 1;
	for (int64_t words = leafSize; words < (p_m >> 1) && k < cacheBase10->size(); words <<= 1) {
		k++;
	}

	// reuse and extend existing table of divisors or create new table as appropriate
	// for b == 10, table overlaps with cacheBase10.table
	r_table.resize(k); // create new table for this conversion
	if (p_b == 10) {
		cacheBase10_mutex.lock();
		for (uint32_t i = 0; i < k; i++) {
			r_table[i] = cacheBase10->at(i); // reuse old table for this conversion
		}
	}

	// extend table
	if (r_table[k - 1].ndigits == 0) {
		// add new entries as needed
		BigNat larger;
		for (uint32_t i = 0; i < k; i++) {
			if (r_table[i].ndigits == 0) {
				if (i == 0) {
					r_table[0].bbb.expWW(p_bb, static_cast<BigWord>(leafSize));
					r_table[0].ndigits = p_ndigits * leafSize;
				} else {
					r_table[i].bbb.sqr(r_table[i - 1].bbb);
					r_table[i].ndigits = 2 * r_table[i - 1].ndigits;
				}

				// optimization: exploit aggregated extra bits in macro blocks
				larger = r_table[i].bbb;
				while (mulAddVWW(larger.slice(), larger.cslice(), p_b, 0) == 0) {
					r_table[i].bbb = larger;
					r_table[i].ndigits++;
				}

				r_table[i].nbits = r_table[i].bbb.bitLen();

				if (p_b == 10) {
					cacheBase10->at(i) = r_table[i];
				}
			}
		}
	}

	if (p_b == 10) {
		cacheBase10_mutex.unlock();
	}
}
