// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_int.h"
#include "godot_big_naturals.h"
#include <godot_cpp/classes/random_number_generator.hpp>

// ProbablyPrime reports whether x is probably prime,
// applying the Miller-Rabin test with n pseudorandomly chosen bases
// as well as a Baillie-PSW test.
//
// If x is prime, ProbablyPrime returns true.
// If x is chosen randomly and not prime, ProbablyPrime probably returns false.
// The probability of returning true for a randomly chosen non-prime is at most ¼ⁿ.
//
// ProbablyPrime is 100% accurate for inputs less than 2⁶⁴.
// See Menezes et al., Handbook of Applied Cryptography, 1997, pp. 145-149,
// and FIPS 186-4 Appendix F for further discussion of the error probabilities.
//
// ProbablyPrime is not suitable for judging primes that an adversary may
// have crafted to fool the test.
//
// As of Go 1.8, ProbablyPrime(0) is allowed and applies only a Baillie-PSW test.
// Before Go 1.8, ProbablyPrime applied only the Miller-Rabin tests, and ProbablyPrime(0) panicked.
bool BigInt::ProbablyPrime(int64_t n) const {
	// Note regarding the doc comment above:
	// It would be more precise to say that the Baillie-PSW test uses the
	// extra strong Lucas test as its Lucas test, but since no one knows
	// how to tell any of the Lucas tests apart inside a Baillie-PSW test
	// (they all work equally well empirically), that detail need not be
	// documented or implicitly guaranteed.
	// The comment does avoid saying "the" Baillie-PSW test
	// because of this general ambiguity.

	ERR_FAIL_COND_V_MSG(n < 0, false, "negative n for ProbablyPrime");
	if (_neg || _abs.is_empty()) {
		return false;
	}

	// primeBitMask records the primes < 64.
	static constexpr uint64_t primeBitMask = (1LLU << 2) | (1LLU << 3) | (1LLU << 5) | (1LLU << 7) |
		(1LLU << 11) | (1LLU << 13) | (1LLU << 17) | (1LLU << 19) | (1LLU << 23) | (1LLU << 29) | (1LLU << 31) |
		(1LLU << 37) | (1LLU << 41) | (1LLU << 43) | (1LLU << 47) | (1LLU << 53) | (1LLU << 59) | (1LLU << 61);

	const uint64_t w = _abs[0];
	if (_abs.size() == 1 && w < 64) {
		return (primeBitMask & (1LLU << w)) != 0;
	}

	if ((w & 1) == 0) {
		return false; // x is even
	}

	static constexpr uint64_t primesA = 3LLU * 5LLU * 7LLU * 11LLU * 13LLU * 17LLU * 19LLU * 23LLU * 37LLU;
	static constexpr uint64_t primesB = 29LLU * 31LLU * 41LLU * 43LLU * 47LLU * 53LLU;

	const uint64_t r = nat_modW(_abs, primesA * primesB);
	const uint32_t rA = uint32_t(r % primesA);
	const uint32_t rB = uint32_t(r % primesB);

	if (rA%3 == 0 || rA%5 == 0 || rA%7 == 0 || rA%11 == 0 || rA%13 == 0 || rA%17 == 0 || rA%19 == 0 || rA%23 == 0 || rA%37 == 0 ||
		rB%29 == 0 || rB%31 == 0 || rB%41 == 0 || rB%43 == 0 || rB%47 == 0 || rB%53 == 0) {
		return false;
	}

	return nat_probablyPrimeMillerRabin(_abs, n + 1, true) && nat_probablyPrimeLucas(_abs);
}

// probablyPrimeMillerRabin reports whether n passes reps rounds of the
// Miller-Rabin primality test, using pseudo-randomly chosen bases.
// If force2 is true, one of the rounds is forced to use base 2.
// See Handbook of Applied Cryptography, p. 139, Algorithm 4.24.
// The number n is known to be non-zero.
bool nat_probablyPrimeMillerRabin(PackedInt64Array n, int64_t reps, bool force2) {
	PackedInt64Array nm1;
	nat_sub(nm1, n, *natOne);

	// determine q, k such that nm1 = q << k
	uint64_t k = nat_trailingZeroBits(nm1);

	PackedInt64Array q;
	nat_rsh(q, nm1, k);

	PackedInt64Array nm3;
	nat_sub(nm3, nm1, *natTwo);

	Ref<RandomNumberGenerator> rng;
	rng.instantiate();
	rng->set_seed(n[0]);
	const Callable rand(*rng, "nexti");

	PackedInt64Array x, y, quotient;
	const int64_t nm3Len = nat_bitLen(nm3);

	for (int64_t i = 0; i < reps; i++) {
		if (i == reps - 1 && force2) {
			nat_set(x, *natTwo);
		} else {
			nat_random(x, rand, nm3, nm3Len);
			nat_add(x, x, *natTwo);
		}

		nat_expNN(y, x, q, n, false);
		if (nat_cmp(y, *natOne) == 0 || nat_cmp(y, nm1) == 0) {
			continue;
		}

		for (uint64_t j = 1; j < k; j++) {
			nat_sqr(y, y);
			nat_div(y, n, quotient, y);
			if (nat_cmp(y, nm1) == 0) {
				goto NextRandom;
			}
			if (nat_cmp(y, *natOne) == 0) {
				return false;
			}
		}

		return false;
NextRandom:;
	}

	return true;
}

// probablyPrimeLucas reports whether n passes the "almost extra strong" Lucas probable prime test,
// using Baillie-OEIS parameter selection. This corresponds to "AESLPSP" on Jacobsen's tables (link below).
// The combination of this test and a Miller-Rabin/Fermat test with base 2 gives a Baillie-PSW test.
//
// References:
//
// Baillie and Wagstaff, "Lucas Pseudoprimes", Mathematics of Computation 35(152),
// October 1980, pp. 1391-1417, especially page 1401.
// https://www.ams.org/journals/mcom/1980-35-152/S0025-5718-1980-0583518-6/S0025-5718-1980-0583518-6.pdf
//
// Grantham, "Frobenius Pseudoprimes", Mathematics of Computation 70(234),
// March 2000, pp. 873-891.
// https://www.ams.org/journals/mcom/2001-70-234/S0025-5718-00-01197-2/S0025-5718-00-01197-2.pdf
//
// Baillie, "Extra strong Lucas pseudoprimes", OEIS A217719, https://oeis.org/A217719.
//
// Jacobsen, "Pseudoprime Statistics, Tables, and Data", http://ntheory.org/pseudoprimes.html.
//
// Nicely, "The Baillie-PSW Primality Test", https://web.archive.org/web/20191121062007/http://www.trnicely.net/misc/bpsw.html.
// (Note that Nicely's definition of the "extra strong" test gives the wrong Jacobi condition,
// as pointed out by Jacobsen.)
//
// Crandall and Pomerance, Prime Numbers: A Computational Perspective, 2nd ed.
// Springer, 2005.
bool nat_probablyPrimeLucas(PackedInt64Array n) {
	// Discard 0, 1.
	if (n.is_empty() || nat_cmp(n, *natOne) == 0) {
		return false;
	}

	// Two is the only even prime.
	// Already checked by caller, but here to allow testing in isolation.
	if ((n[0] & 1) == 0) {
		return nat_cmp(n, *natTwo) == 0;
	}

	// Baillie-OEIS "method C" for choosing D, P, Q,
	// as in https://oeis.org/A217719/a217719.txt:
	// try increasing P ≥ 3 such that D = P² - 4 (so Q = 1)
	// until Jacobi(D, n) = -1.
	// The search is expected to succeed for non-square n after just a few trials.
	// After more than expected failures, check whether n is square
	// (which would cause Jacobi(D, n) = 1 for all D not dividing n).
	uint64_t p = 3;
	PackedInt64Array d{1};
	PackedInt64Array t1; // temp
	Ref<BigInt> intD, intN;
	intD.instantiate();
	intD->_set_abs(d);
	intN.instantiate();
	intN->_set_abs(n);
	for (; ; p++) {
		if (p > 10000) {
			// This is widely believed to be impossible.
			// If we get a report, we'll want the exact number n.
			ERR_FAIL_V_MSG(false, vformat("math/big: internal error: cannot find (D/n) = -1 for %s", intN));
		}

		d[0] = p*p - 4;
		intD->_set_abs(d);

		int j = BigInt::Jacobi(intD, intN);
		if (j == -1) {
			break;
		}

		if (j == 0) {
			// d = p²-4 = (p-2)(p+2).
			// If (d/n) == 0 then d shares a prime factor with n.
			// Since the loop proceeds in increasing p and starts with p-2==1,
			// the shared prime factor must be p+2.
			// If p+2 == n, then n is prime; otherwise p+2 is a proper factor of n.
			return n.size() == 1 && n[0] == p + 2;
		}

		if (p == 40) {
			// We'll never find (d/n) = -1 if n is a square.
			// If n is a non-square we expect to find a d in just a few attempts on average.
			// After 40 attempts, take a moment to check if n is indeed a square.
			nat_sqrt(t1, n);
			nat_sqr(t1, t1);
			if (nat_cmp(t1, n) == 0) {
				return false;
			}
		}
	}

	// Grantham definition of "extra strong Lucas pseudoprime", after Thm 2.3 on p. 876
	// (D, P, Q above have become Δ, b, 1):
	//
	// Let U_n = U_n(b, 1), V_n = V_n(b, 1), and Δ = b²-4.
	// An extra strong Lucas pseudoprime to base b is a composite n = 2^r s + Jacobi(Δ, n),
	// where s is odd and gcd(n, 2*Δ) = 1, such that either (i) U_s ≡ 0 mod n and V_s ≡ ±2 mod n,
	// or (ii) V_{2^t s} ≡ 0 mod n for some 0 ≤ t < r-1.
	//
	// We know gcd(n, Δ) = 1 or else we'd have found Jacobi(d, n) == 0 above.
	// We know gcd(n, 2) = 1 because n is odd.
	//
	// Arrange s = (n - Jacobi(Δ, n)) / 2^r = (n+1) / 2^r.
	PackedInt64Array s;
	nat_add(s, n, *natOne);
	const uint64_t r = nat_trailingZeroBits(s);
	nat_rsh(s, s, r);
	PackedInt64Array nm2; // n-2
	nat_sub(nm2, n, *natTwo);

	// We apply the "almost extra strong" test, which checks the above conditions
	// except for U_s ≡ 0 mod n, which allows us to avoid computing any U_k values.
	// Jacobsen points out that maybe we should just do the full extra strong test:
	// "It is also possible to recover U_n using Crandall and Pomerance equation 3.13:
	// U_n = D^-1 (2V_{n+1} - PV_n) allowing us to run the full extra-strong test
	// at the cost of a single modular inversion. This computation is easy and fast in GMP,
	// so we can get the full extra-strong test at essentially the same performance as the
	// almost extra strong test."

	// Compute Lucas sequence V_s(b, 1), where:
	//
	//	V(0) = 2
	//	V(1) = P
	//	V(k) = P V(k-1) - Q V(k-2).
	//
	// (Remember that due to method C above, P = b, Q = 1.)
	//
	// In general V(k) = α^k + β^k, where α and β are roots of x² - Px + Q.
	// Crandall and Pomerance (p.147) observe that for 0 ≤ j ≤ k,
	//
	//	V(j+k) = V(j)V(k) - V(k-j).
	//
	// So in particular, to quickly double the subscript:
	//
	//	V(2k) = V(k)² - 2
	//	V(2k+1) = V(k) V(k+1) - P
	//
	// We can therefore start with k=0 and build up to k=s in log₂(s) steps.
	PackedInt64Array natP, vk, vk1, t2;
	nat_setWord(natP, p);
	nat_setWord(vk, 2);
	nat_setWord(vk1, p);
	for (int64_t i = nat_bitLen(s); i >= 0; i--) {
		if (nat_bit(s, uint64_t(i)) != 0) {
			// k' = 2k+1
			// V(k') = V(2k+1) = V(k) V(k+1) - P.
			nat_mul(t1, vk, vk1);
			nat_add(t1, t1, n);
			nat_sub(t1, t1, natP);
			nat_div(t1, n, t2, vk);
			// V(k'+1) = V(2k+2) = V(k+1)² - 2.
			nat_sqr(t1, vk1);
			nat_add(t1, t1, nm2);
			nat_div(t1, n, t2, vk1);
		} else {
			// k' = 2k
			// V(k'+1) = V(2k+1) = V(k) V(k+1) - P.
			nat_mul(t1, vk, vk1);
			nat_add(t1, t1, n);
			nat_sub(t1, t1, natP);
			nat_div(t1, n, t2, vk1);
			// V(k') = V(2k) = V(k)² - 2
			nat_sqr(t1, vk);
			nat_add(t1, t1, nm2);
			nat_div(t1, n, t2, vk);
		}
	}

	// Now k=s, so vk = V(s). Check V(s) ≡ ±2 (mod n).
	if (nat_cmp(vk, *natTwo) == 0 || nat_cmp(vk, nm2) == 0) {
		// Check U(s) ≡ 0.
		// As suggested by Jacobsen, apply Crandall and Pomerance equation 3.13:
		//
		//	U(k) = D⁻¹ (2 V(k+1) - P V(k))
		//
		// Since we are checking for U(k) == 0 it suffices to check 2 V(k+1) == P V(k) mod n,
		// or P V(k) - 2 V(k+1) == 0 mod n.
		nat_mul(t1, vk, natP);
		nat_lsh(t2, vk1, 1);
		if (nat_cmp(t1, t2) < 0) {
			std::swap(t1, t2);
		}
		nat_sub(t1, t1, t2);
		PackedInt64Array t3;
		nat_div(t1, n, t2, t3);
		if (t3.is_empty()) {
			return true;
		}
	}

	// Check V(2^t s) ≡ 0 mod n for some 0 ≤ t < r-1.
	for (uint64_t t = 0; t < r - 1; t++) {
		if (vk.is_empty()) { // vk == 0
			return true;
		}
		// Optimization: V(k) = 2 is a fixed point for V(k') = V(k)² - 2,
		// so if V(k) = 2, we can stop: we will never find a future V(k) == 0.
		if (vk.size() == 1 && vk[0] == 2) { // vk == 2
			return false;
		}
		// k' = 2k
		// V(k') = V(2k) = V(k)² - 2
		nat_sqr(t1, vk);
		nat_sub(t1, t1, *natTwo);
		nat_div(t1, n, t2, vk);
	}

	return false;
}
