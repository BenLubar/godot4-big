// This file is ported from src/math/big/prime.go in Go 1.26.1.
// Original copyright notice follows:

// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_int.h"

#include <godot_cpp/classes/random_number_generator.hpp>

using namespace godot;

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
bool BigInt::ProbablyPrime(int64_t p_n) const {
	// Note regarding the doc comment above:
	// It would be more precise to say that the Baillie-PSW test uses the
	// extra strong Lucas test as its Lucas test, but since no one knows
	// how to tell any of the Lucas tests apart inside a Baillie-PSW test
	// (they all work equally well empirically), that detail need not be
	// documented or implicitly guaranteed.
	// The comment does avoid saying "the" Baillie-PSW test
	// because of this general ambiguity.

	ERR_FAIL_COND_V_MSG(p_n < 0, false, "negative n for ProbablyPrime");
	if (_neg || _abs.array.is_empty()) {
		return false;
	}

	// primeBitMask records the primes < 64.
	constexpr uint64_t primeBitMask = (1LLU<<2) | (1LLU<<3) | (1LLU<<5) | (1LLU<<7) |
		(1LLU<<11) | (1LLU<<13) | (1LLU<<17) | (1LLU<<19) | (1LLU<<23) | (1LLU<<29) | (1LLU<<31) |
		(1LLU<<37) | (1LLU<<41) | (1LLU<<43) | (1LLU<<47) | (1LLU<<53) | (1LLU<<59) | (1LLU<<61);

	BigWord w = _abs[0];
	if (_abs.array.size() == 1 && w < 64) {
		return (primeBitMask & (1LLU << w)) != 0;
	}

	if ((w & 1) == 0) {
		return false; // x is even
	}

	constexpr uint64_t primesA = 3LLU * 5LLU * 7LLU * 11LLU * 13LLU * 17LLU * 19LLU * 23LLU * 37LLU;
	constexpr uint64_t primesB = 29LLU * 31LLU * 41LLU * 43LLU * 47LLU * 53LLU;

	const BigWord r = _abs.modW(primesA * primesB);
	uint32_t rA = uint32_t(r % primesA);
	uint32_t rB = uint32_t(r % primesB);

	if (rA%3 == 0 || rA%5 == 0 || rA%7 == 0 || rA%11 == 0 || rA%13 == 0 || rA%17 == 0 || rA%19 == 0 || rA%23 == 0 || rA%37 == 0 ||
		rB%29 == 0 || rB%31 == 0 || rB%41 == 0 || rB%43 == 0 || rB%47 == 0 || rB%53 == 0) {
		return false;
	}

	return _abs.probablyPrimeMillerRabin(p_n + 1, true) && _abs.probablyPrimeLucas();
}

// probablyPrimeMillerRabin reports whether n passes reps rounds of the
// Miller-Rabin primality test, using pseudo-randomly chosen bases.
// If force2 is true, one of the rounds is forced to use base 2.
// See Handbook of Applied Cryptography, p. 139, Algorithm 4.24.
// The number n is known to be non-zero.
bool BigNat::probablyPrimeMillerRabin(int64_t p_reps, bool p_force2) const {
	BigNat nm1;
	nm1.sub(*this, BigNat{{1}});
	// determine q, k such that nm1 = q << k
	uint64_t k = nm1.trailingZeroBits();
	BigNat q;
	q.rsh(nm1, k);

	BigNat nm3;
	nm3.sub(nm1, BigNat{{2}});

	Ref<RandomNumberGenerator> rand;
	rand.instantiate();
	rand->set_seed(VariantHasher::hash(array));

	BigNat x, y, quotient;
	int64_t nm3Len = nm3.bitLen();

	for (int64_t i = 0; i < p_reps; i++) {
		if (i == p_reps - 1 && p_force2) {
			x.setUint64(2);
		} else {
			x.random([rand]() -> uint32_t { return rand->randi(); }, nm3, nm3Len);
			x.add(x, BigNat{{2}});
		}

		y.expNN(x, q, *this, false);
		if (y.cmp(BigNat{{1}}) == 0 || y.cmp(nm1) == 0) {
			continue;
		}

		for (uint64_t j = 1; j < k; j++) {
			y.sqr(y);
			quotient.div(y, y, *this);

			if (y.cmp(nm1) == 0) {
				goto NextRandom;
			}

			if (y.cmp(BigNat{{1}}) == 0) {
				return false;
			}
		}

		return false;
NextRandom:
		;
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
bool BigNat::probablyPrimeLucas() const {
	// Discard 0, 1.
	if (cmp(BigNat{{1}}) <= 0) {
		return false;
	}

	// Two is the only even prime.
	// Already checked by caller, but here to allow testing in isolation.
	if ((array[0] & 1) == 0) {
		return cmp(BigNat{{2}}) == 0;
	}

	// Baillie-OEIS "method C" for choosing D, P, Q,
	// as in https://oeis.org/A217719/a217719.txt:
	// try increasing P ≥ 3 such that D = P² - 4 (so Q = 1)
	// until Jacobi(D, n) = -1.
	// The search is expected to succeed for non-square n after just a few trials.
	// After more than expected failures, check whether n is square
	// (which would cause Jacobi(D, n) = 1 for all D not dividing n).
	BigWord p = 3;
	BigNat t1; // temp
	Ref<BigInt> intD, intN;
	intD.instantiate();
	intD->_abs = BigNat{{1}};
	intN.instantiate();
	intN->_abs = *this;
	BigNat &d = intD->_abs;
	for (; ; p++) {
		// This is widely believed to be impossible.
		// If we get a report, we'll want the exact number n.
		CRASH_COND_MSG(p > 10000, "math/big: internal error: cannot find (D/n) = -1 for " + intN->String());
		d[0] = p * p - 4;
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
			return array.size() == 1 && array[0] == p + 2;
		}

		if (p == 40) {
			// We'll never find (d/n) = -1 if n is a square.
			// If n is a non-square we expect to find a d in just a few attempts on average.
			// After 40 attempts, take a moment to check if n is indeed a square.
			t1.sqrt(*this);
			t1.sqr(t1);
			if (t1.cmp(*this) == 0) {
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
	BigNat s, nm2;
	s.add(*this, BigNat{{1}});
	int64_t r = s.trailingZeroBits();
	s.rsh(s, uint64_t(r));
	nm2.sub(*this, BigNat{{2}}); // n-2

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
	BigNat natP, vk, vk1, t2;
	natP.setUint64(p);
	vk.setUint64(2);
	vk1.setUint64(p);
	for (int64_t i = s.bitLen(); i >= 0; i--) {
		if (s.bit(uint64_t(i)) != 0) {
			// k' = 2k+1
			// V(k') = V(2k+1) = V(k) V(k+1) - P.
			t1.mul(vk, vk1);
			t1.add(t1, *this);
			t1.sub(t1, natP);
			t2.div(vk, t1, *this);
			// V(k'+1) = V(2k+2) = V(k+1)² - 2.
			t1.sqr(vk1);
			t1.add(t1, nm2);
			t2.div(vk1, t1, *this);
		} else {
			// k' = 2k
			// V(k'+1) = V(2k+1) = V(k) V(k+1) - P.
			t1.mul(vk, vk1);
			t1.add(t1, *this);
			t1.sub(t1, natP);
			t2.div(vk1, t1, *this);
			// V(k') = V(2k) = V(k)² - 2
			t1.sqr(vk);
			t1.add(t1, nm2);
			t2.div(vk, t1, *this);
		}
	}

	// Now k=s, so vk = V(s). Check V(s) ≡ ±2 (mod n).
	if (vk.cmp(BigNat{{2}}) == 0 || vk.cmp(nm2) == 0) {
		// Check U(s) ≡ 0.
		// As suggested by Jacobsen, apply Crandall and Pomerance equation 3.13:
		//
		//	U(k) = D⁻¹ (2 V(k+1) - P V(k))
		//
		// Since we are checking for U(k) == 0 it suffices to check 2 V(k+1) == P V(k) mod n,
		// or P V(k) - 2 V(k+1) == 0 mod n.
		t1.mul(vk, natP);
		t2.lsh(vk1, 1);
		if (t1.cmp(t2) < 0) {
			SWAP(t1, t2);
		}
		t1.sub(t1, t2);
		BigNat &t3 = vk1; // steal vk1, no longer needed below
		t2.div(t3, t1, *this);
		if (t3.array.is_empty()) {
			return true;
		}
	}

	// Check V(2^t s) ≡ 0 mod n for some 0 ≤ t < r-1.
	for (int64_t t = 0; t < r - 1; t++) {
		if (vk.array.is_empty()) { // vk == 0
			return true;
		}
		// Optimization: V(k) = 2 is a fixed point for V(k') = V(k)² - 2,
		// so if V(k) = 2, we can stop: we will never find a future V(k) == 0.
		if (vk.array.size() == 1 && vk[0] == 2) { // vk == 2
			return false;
		}
		// k' = 2k
		// V(k') = V(2k) = V(k)² - 2
		t1.sqr(vk);
		t1.sub(t1, BigNat{{2}});
		t2.div(vk, t1, *this);
	}

	return false;
}
