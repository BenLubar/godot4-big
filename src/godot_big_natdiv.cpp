// This file was ported from Go 1.25.7. Original copyright notice follows:

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "godot_big_naturals.h"

/*

Multi-precision division. Here be dragons.

Given u and v, where u is n+m digits, and v is n digits (with no leading zeros),
the goal is to return quo, rem such that u = quo*v + rem, where 0 ≤ rem < v.
That is, quo = ⌊u/v⌋ where ⌊x⌋ denotes the floor (truncation to integer) of x,
and rem = u - quo·v.


Long Division

Division in a computer proceeds the same as long division in elementary school,
but computers are not as good as schoolchildren at following vague directions,
so we have to be much more precise about the actual steps and what can happen.

We work from most to least significant digit of the quotient, doing:

 • Guess a digit q, the number of v to subtract from the current
   section of u to zero out the topmost digit.
 • Check the guess by multiplying q·v and comparing it against
   the current section of u, adjusting the guess as needed.
 • Subtract q·v from the current section of u.
 • Add q to the corresponding section of the result quo.

When all digits have been processed, the final remainder is left in u
and returned as rem.

For example, here is a sketch of dividing 5 digits by 3 digits (n=3, m=2).

	                 q₂ q₁ q₀
	         _________________
	v₂ v₁ v₀ ) u₄ u₃ u₂ u₁ u₀
	           ↓  ↓  ↓  |  |
	          [u₄ u₃ u₂]|  |
	        - [  q₂·v  ]|  |
	        ----------- ↓  |
	          [  rem  | u₁]|
	        - [    q₁·v   ]|
	           ----------- ↓
	             [  rem  | u₀]
	           - [    q₀·v   ]
	              ------------
	                [  rem   ]

Instead of creating new storage for the remainders and copying digits from u
as indicated by the arrows, we use u's storage directly as both the source
and destination of the subtractions, so that the remainders overwrite
successive overlapping sections of u as the division proceeds, using a slice
of u to identify the current section. This avoids all the copying as well as
shifting of remainders.

Division of u with n+m digits by v with n digits (in base B) can in general
produce at most m+1 digits, because:

  • u < B^(n+m)               [B^(n+m) has n+m+1 digits]
  • v ≥ B^(n-1)               [B^(n-1) is the smallest n-digit number]
  • u/v < B^(n+m) / B^(n-1)   [divide bounds for u, v]
  • u/v < B^(m+1)             [simplify]

The first step is special: it takes the top n digits of u and divides them by
the n digits of v, producing the first quotient digit and an n-digit remainder.
In the example, q₂ = ⌊u₄u₃u₂ / v⌋.

The first step divides n digits by n digits to ensure that it produces only a
single digit.

Each subsequent step appends the next digit from u to the remainder and divides
those n+1 digits by the n digits of v, producing another quotient digit and a
new n-digit remainder.

Subsequent steps divide n+1 digits by n digits, an operation that in general
might produce two digits. However, as used in the algorithm, that division is
guaranteed to produce only a single digit. The dividend is of the form
rem·B + d, where rem is a remainder from the previous step and d is a single
digit, so:

 • rem ≤ v - 1                 [rem is a remainder from dividing by v]
 • rem·B ≤ v·B - B             [multiply by B]
 • d ≤ B - 1                   [d is a single digit]
 • rem·B + d ≤ v·B - 1         [add]
 • rem·B + d < v·B             [change ≤ to <]
 • (rem·B + d)/v < B           [divide by v]


Guess and Check

At each step we need to divide n+1 digits by n digits, but this is for the
implementation of division by n digits, so we can't just invoke a division
routine: we _are_ the division routine. Instead, we guess at the answer and
then check it using multiplication. If the guess is wrong, we correct it.

How can this guessing possibly be efficient? It turns out that the following
statement (let's call it the Good Guess Guarantee) is true.

If

 • q = ⌊u/v⌋ where u is n+1 digits and v is n digits,
 • q < B, and
 • the topmost digit of v = vₙ₋₁ ≥ B/2,

then q̂ = ⌊uₙuₙ₋₁ / vₙ₋₁⌋ satisfies q ≤ q̂ ≤ q+2. (Proof below.)

That is, if we know the answer has only a single digit and we guess an answer
by ignoring the bottom n-1 digits of u and v, using a 2-by-1-digit division,
then that guess is at least as large as the correct answer. It is also not
too much larger: it is off by at most two from the correct answer.

Note that in the first step of the overall division, which is an n-by-n-digit
division, the 2-by-1 guess uses an implicit uₙ = 0.

Note that using a 2-by-1-digit division here does not mean calling ourselves
recursively. Instead, we use an efficient direct hardware implementation of
that operation.

Note that because q is u/v rounded down, q·v must not exceed u: u ≥ q·v.
If a guess q̂ is too big, it will not satisfy this test. Viewed a different way,
the remainder r̂ for a given q̂ is u - q̂·v, which must be positive. If it is
negative, then the guess q̂ is too big.

This gives us a way to compute q. First compute q̂ with 2-by-1-digit division.
Then, while u < q̂·v, decrement q̂; this loop executes at most twice, because
q̂ ≤ q+2.


Scaling Inputs

The Good Guess Guarantee requires that the top digit of v (vₙ₋₁) be at least B/2.
For example in base 10, ⌊172/19⌋ = 9, but ⌊18/1⌋ = 18: the guess is wildly off
because the first digit 1 is smaller than B/2 = 5.

We can ensure that v has a large top digit by multiplying both u and v by the
right amount. Continuing the example, if we multiply both 172 and 19 by 3, we
now have ⌊516/57⌋, the leading digit of v is now ≥ 5, and sure enough
⌊51/5⌋ = 10 is much closer to the correct answer 9. It would be easier here
to multiply by 4, because that can be done with a shift. Specifically, we can
always count the number of leading zeros i in the first digit of v and then
shift both u and v left by i bits.

Having scaled u and v, the value ⌊u/v⌋ is unchanged, but the remainder will
be scaled: 172 mod 19 is 1, but 516 mod 57 is 3. We have to divide the remainder
by the scaling factor (shifting right i bits) when we finish.

Note that these shifts happen before and after the entire division algorithm,
not at each step in the per-digit iteration.

Note the effect of scaling inputs on the size of the possible quotient.
In the scaled u/v, u can gain a digit from scaling; v never does, because we
pick the scaling factor to make v's top digit larger but without overflowing.
If u and v have n+m and n digits after scaling, then:

  • u < B^(n+m)               [B^(n+m) has n+m+1 digits]
  • v ≥ B^n / 2               [vₙ₋₁ ≥ B/2, so vₙ₋₁·B^(n-1) ≥ B^n/2]
  • u/v < B^(n+m) / (B^n / 2) [divide bounds for u, v]
  • u/v < 2 B^m               [simplify]

The quotient can still have m+1 significant digits, but if so the top digit
must be a 1. This provides a different way to handle the first digit of the
result: compare the top n digits of u against v and fill in either a 0 or a 1.


Refining Guesses

Before we check whether u < q̂·v, we can adjust our guess to change it from
q̂ = ⌊uₙuₙ₋₁ / vₙ₋₁⌋ into the refined guess ⌊uₙuₙ₋₁uₙ₋₂ / vₙ₋₁vₙ₋₂⌋.
Although not mentioned above, the Good Guess Guarantee also promises that this
3-by-2-digit division guess is more precise and at most one away from the real
answer q. The improvement from the 2-by-1 to the 3-by-2 guess can also be done
without n-digit math.

If we have a guess q̂ = ⌊uₙuₙ₋₁ / vₙ₋₁⌋ and we want to see if it also equal to
⌊uₙuₙ₋₁uₙ₋₂ / vₙ₋₁vₙ₋₂⌋, we can use the same check we would for the full division:
if uₙuₙ₋₁uₙ₋₂ < q̂·vₙ₋₁vₙ₋₂, then the guess is too large and should be reduced.

Checking uₙuₙ₋₁uₙ₋₂ < q̂·vₙ₋₁vₙ₋₂ is the same as uₙuₙ₋₁uₙ₋₂ - q̂·vₙ₋₁vₙ₋₂ < 0,
and

	uₙuₙ₋₁uₙ₋₂ - q̂·vₙ₋₁vₙ₋₂ = (uₙuₙ₋₁·B + uₙ₋₂) - q̂·(vₙ₋₁·B + vₙ₋₂)
	                          [splitting off the bottom digit]
	                      = (uₙuₙ₋₁ - q̂·vₙ₋₁)·B + uₙ₋₂ - q̂·vₙ₋₂
	                          [regrouping]

The expression (uₙuₙ₋₁ - q̂·vₙ₋₁) is the remainder of uₙuₙ₋₁ / vₙ₋₁.
If the initial guess returns both q̂ and its remainder r̂, then checking
whether uₙuₙ₋₁uₙ₋₂ < q̂·vₙ₋₁vₙ₋₂ is the same as checking r̂·B + uₙ₋₂ < q̂·vₙ₋₂.

If we find that r̂·B + uₙ₋₂ < q̂·vₙ₋₂, then we can adjust the guess by
decrementing q̂ and adding vₙ₋₁ to r̂. We repeat until r̂·B + uₙ₋₂ ≥ q̂·vₙ₋₂.
(As before, this fixup is only needed at most twice.)

Now that q̂ = ⌊uₙuₙ₋₁uₙ₋₂ / vₙ₋₁vₙ₋₂⌋, as mentioned above it is at most one
away from the correct q, and we've avoided doing any n-digit math.
(If we need the new remainder, it can be computed as r̂·B + uₙ₋₂ - q̂·vₙ₋₂.)

The final check u < q̂·v and the possible fixup must be done at full precision.
For random inputs, a fixup at this step is exceedingly rare: the 3-by-2 guess
is not often wrong at all. But still we must do the check. Note that since the
3-by-2 guess is off by at most 1, it can be convenient to perform the final
u < q̂·v as part of the computation of the remainder r = u - q̂·v. If the
subtraction underflows, decremeting q̂ and adding one v back to r is enough to
arrive at the final q, r.

That's the entirety of long division: scale the inputs, and then loop over
each output position, guessing, checking, and correcting the next output digit.

For a 2n-digit number divided by an n-digit number (the worst size-n case for
division complexity), this algorithm uses n+1 iterations, each of which must do
at least the 1-by-n-digit multiplication q̂·v. That's O(n) iterations of
O(n) time each, so O(n²) time overall.


Recursive Division

For very large inputs, it is possible to improve on the O(n²) algorithm.
Let's call a group of n/2 real digits a (very) “wide digit”. We can run the
standard long division algorithm explained above over the wide digits instead of
the actual digits. This will result in many fewer steps, but the math involved in
each step is more work.

Where basic long division uses a 2-by-1-digit division to guess the initial q̂,
the new algorithm must use a 2-by-1-wide-digit division, which is of course
really an n-by-n/2-digit division. That's OK: if we implement n-digit division
in terms of n/2-digit division, the recursion will terminate when the divisor
becomes small enough to handle with standard long division or even with the
2-by-1 hardware instruction.

For example, here is a sketch of dividing 10 digits by 4, proceeding with
wide digits corresponding to two regular digits. The first step, still special,
must leave off a (regular) digit, dividing 5 by 4 and producing a 4-digit
remainder less than v. The middle steps divide 6 digits by 4, guaranteed to
produce two output digits each (one wide digit) with 4-digit remainders.
The final step must use what it has: the 4-digit remainder plus one more,
5 digits to divide by 4.

	                       q₆ q₅ q₄ q₃ q₂ q₁ q₀
	            _______________________________
	v₃ v₂ v₁ v₀ ) u₉ u₈ u₇ u₆ u₅ u₄ u₃ u₂ u₁ u₀
	              ↓  ↓  ↓  ↓  ↓  |  |  |  |  |
	             [u₉ u₈ u₇ u₆ u₅]|  |  |  |  |
	           - [    q₆q₅·v    ]|  |  |  |  |
	           ----------------- ↓  ↓  |  |  |
	                [    rem    |u₄ u₃]|  |  |
	              - [     q₄q₃·v      ]|  |  |
	              -------------------- ↓  ↓  |
	                      [    rem    |u₂ u₁]|
	                    - [     q₂q₁·v      ]|
	                    -------------------- ↓
	                            [    rem    |u₀]
	                          - [     q₀·v     ]
	                          ------------------
	                               [    rem    ]

An alternative would be to look ahead to how well n/2 divides into n+m and
adjust the first step to use fewer digits as needed, making the first step
more special to make the last step not special at all. For example, using the
same input, we could choose to use only 4 digits in the first step, leaving
a full wide digit for the last step:

	                       q₆ q₅ q₄ q₃ q₂ q₁ q₀
	            _______________________________
	v₃ v₂ v₁ v₀ ) u₉ u₈ u₇ u₆ u₅ u₄ u₃ u₂ u₁ u₀
	              ↓  ↓  ↓  ↓  |  |  |  |  |  |
	             [u₉ u₈ u₇ u₆]|  |  |  |  |  |
	           - [    q₆·v   ]|  |  |  |  |  |
	           -------------- ↓  ↓  |  |  |  |
	             [    rem    |u₅ u₄]|  |  |  |
	           - [     q₅q₄·v      ]|  |  |  |
	           -------------------- ↓  ↓  |  |
	                   [    rem    |u₃ u₂]|  |
	                 - [     q₃q₂·v      ]|  |
	                 -------------------- ↓  ↓
	                         [    rem    |u₁ u₀]
	                       - [     q₁q₀·v      ]
	                       ---------------------
	                               [    rem    ]

Today, the code in divRecursiveStep works like the first example. Perhaps in
the future we will make it work like the alternative, to avoid a special case
in the final iteration.

Either way, each step is a 3-by-2-wide-digit division approximated first by
a 2-by-1-wide-digit division, just as we did for regular digits in long division.
Because the actual answer we want is a 3-by-2-wide-digit division, instead of
multiplying q̂·v directly during the fixup, we can use the quick refinement
from long division (an n/2-by-n/2 multiply) to correct q to its actual value
and also compute the remainder (as mentioned above), and then stop after that,
never doing a full n-by-n multiply.

Instead of using an n-by-n/2-digit division to produce n/2 digits, we can add
(not discard) one more real digit, doing an (n+1)-by-(n/2+1)-digit division that
produces n/2+1 digits. That single extra digit tightens the Good Guess Guarantee
to q ≤ q̂ ≤ q+1 and lets us drop long division's special treatment of the first
digit. These benefits are discussed more after the Good Guess Guarantee proof
below.


How Fast is Recursive Division?

For a 2n-by-n-digit division, this algorithm runs a 4-by-2 long division over
wide digits, producing two wide digits plus a possible leading regular digit 1,
which can be handled without a recursive call. That is, the algorithm uses two
full iterations, each using an n-by-n/2-digit division and an n/2-by-n/2-digit
multiplication, along with a few n-digit additions and subtractions. The standard
n-by-n-digit multiplication algorithm requires O(n²) time, making the overall
algorithm require time T(n) where

	T(n) = 2T(n/2) + O(n) + O(n²)

which, by the Bentley-Haken-Saxe theorem, ends up reducing to T(n) = O(n²).
This is not an improvement over regular long division.

When the number of digits n becomes large enough, Karatsuba's algorithm for
multiplication can be used instead, which takes O(n^log₂3) = O(n^1.6) time.
(Karatsuba multiplication is implemented in func karatsuba in nat.go.)
That makes the overall recursive division algorithm take O(n^1.6) time as well,
which is an improvement, but again only for large enough numbers.

It is not critical to make sure that every recursion does only two recursive
calls. While in general the number of recursive calls can change the time
analysis, in this case doing three calls does not change the analysis:

	T(n) = 3T(n/2) + O(n) + O(n^log₂3)

ends up being T(n) = O(n^log₂3). Because the Karatsuba multiplication taking
time O(n^log₂3) is itself doing 3 half-sized recursions, doing three for the
division does not hurt the asymptotic performance. Of course, it is likely
still faster in practice to do two.


Proof of the Good Guess Guarantee

Given numbers x, y, let us break them into the quotients and remainders when
divided by some scaling factor S, with the added constraints that the quotient
x/y and the high part of y are both less than some limit T, and that the high
part of y is at least half as big as T.

	x₁ = ⌊x/S⌋        y₁ = ⌊y/S⌋
	x₀ = x mod S      y₀ = y mod S

	x  = x₁·S + x₀    0 ≤ x₀ < S    x/y < T
	y  = y₁·S + y₀    0 ≤ y₀ < S    T/2 ≤ y₁ < T

And consider the two truncated quotients:

	q = ⌊x/y⌋
	q̂ = ⌊x₁/y₁⌋

We will prove that q ≤ q̂ ≤ q+2.

The guarantee makes no real demands on the scaling factor S: it is simply the
magnitude of the digits cut from both x and y to produce x₁ and y₁.
The guarantee makes only limited demands on T: it must be large enough to hold
the quotient x/y, and y₁ must have roughly the same size.

To apply to the earlier discussion of 2-by-1 guesses in long division,
we would choose:

	S  = Bⁿ⁻¹
	T  = B
	x  = u
	x₁ = uₙuₙ₋₁
	x₀ = uₙ₋₂...u₀
	y  = v
	y₁ = vₙ₋₁
	y₀ = vₙ₋₂...u₀

These simpler variables avoid repeating those longer expressions in the proof.

Note also that, by definition, truncating division ⌊x/y⌋ satisfies

	x/y - 1 < ⌊x/y⌋ ≤ x/y.

This fact will be used a few times in the proofs.

Proof that q ≤ q̂:

	q̂·y₁ = ⌊x₁/y₁⌋·y₁                      [by definition, q̂ = ⌊x₁/y₁⌋]
	     > (x₁/y₁ - 1)·y₁                  [x₁/y₁ - 1 < ⌊x₁/y₁⌋]
	     = x₁ - y₁                         [distribute y₁]

	So q̂·y₁ > x₁ - y₁.
	Since q̂·y₁ is an integer, q̂·y₁ ≥ x₁ - y₁ + 1.

	q̂ - q = q̂ - ⌊x/y⌋                      [by definition, q = ⌊x/y⌋]
	      ≥ q̂ - x/y                        [⌊x/y⌋ < x/y]
	      = (1/y)·(q̂·y - x)                [factor out 1/y]
	      ≥ (1/y)·(q̂·y₁·S - x)             [y = y₁·S + y₀ ≥ y₁·S]
	      ≥ (1/y)·((x₁ - y₁ + 1)·S - x)    [above: q̂·y₁ ≥ x₁ - y₁ + 1]
	      = (1/y)·(x₁·S - y₁·S + S - x)    [distribute S]
	      = (1/y)·(S - x₀ - y₁·S)          [-x = -x₁·S - x₀]
	      > -y₁·S / y                      [x₀ < S, so S - x₀ > 0; drop it]
	      ≥ -1                             [y₁·S ≤ y]

	So q̂ - q > -1.
	Since q̂ - q is an integer, q̂ - q ≥ 0, or equivalently q ≤ q̂.

Proof that q̂ ≤ q+2:

	x₁/y₁ - x/y = x₁·S/y₁·S - x/y          [multiply left term by S/S]
	            ≤ x/y₁·S - x/y             [x₁S ≤ x]
	            = (x/y)·(y/y₁·S - 1)       [factor out x/y]
	            = (x/y)·((y - y₁·S)/y₁·S)  [move -1 into y/y₁·S fraction]
	            = (x/y)·(y₀/y₁·S)          [y - y₁·S = y₀]
	            = (x/y)·(1/y₁)·(y₀/S)      [factor out 1/y₁]
	            < (x/y)·(1/y₁)             [y₀ < S, so y₀/S < 1]
	            ≤ (x/y)·(2/T)              [y₁ ≥ T/2, so 1/y₁ ≤ 2/T]
	            < T·(2/T)                  [x/y < T]
	            = 2                        [T·(2/T) = 2]

	So x₁/y₁ - x/y < 2.

	q̂ - q = ⌊x₁/y₁⌋ - q                    [by definition, q̂ = ⌊x₁/y₁⌋]
	      = ⌊x₁/y₁⌋ - ⌊x/y⌋                [by definition, q = ⌊x/y⌋]
	      ≤ x₁/y₁ - ⌊x/y⌋                  [⌊x₁/y₁⌋ ≤ x₁/y₁]
	      < x₁/y₁ - (x/y - 1)              [⌊x/y⌋ > x/y - 1]
	      = (x₁/y₁ - x/y) + 1              [regrouping]
	      < 2 + 1                          [above: x₁/y₁ - x/y < 2]
	      = 3

	So q̂ - q < 3.
	Since q̂ - q is an integer, q̂ - q ≤ 2.

Note that when x/y < T/2, the bounds tighten to x₁/y₁ - x/y < 1 and therefore
q̂ - q ≤ 1.

Note also that in the general case 2n-by-n division where we don't know that
x/y < T, we do know that x/y < 2T, yielding the bound q̂ - q ≤ 4. So we could
remove the special case first step of long division as long as we allow the
first fixup loop to run up to four times. (Using a simple comparison to decide
whether the first digit is 0 or 1 is still more efficient, though.)

Finally, note that when dividing three leading base-B digits by two (scaled),
we have T = B² and x/y < B = T/B, a much tighter bound than x/y < T.
This in turn yields the much tighter bound x₁/y₁ - x/y < 2/B. This means that
⌊x₁/y₁⌋ and ⌊x/y⌋ can only differ when x/y is less than 2/B greater than an
integer. For random x and y, the chance of this is 2/B, or, for large B,
approximately zero. This means that after we produce the 3-by-2 guess in the
long division algorithm, the fixup loop essentially never runs.

In the recursive algorithm, the extra digit in (2·⌊n/2⌋+1)-by-(⌊n/2⌋+1)-digit
division has exactly the same effect: the probability of needing a fixup is the
same 2/B. Even better, we can allow the general case x/y < 2T and the fixup
probability only grows to 4/B, still essentially zero.


References

There are no great references for implementing long division; thus this comment.
Here are some notes about what to expect from the obvious references.

Knuth Volume 2 (Seminumerical Algorithms) section 4.3.1 is the usual canonical
reference for long division, but that entire series is highly compressed, never
repeating a necessary fact and leaving important insights to the exercises.
For example, no rationale whatsoever is given for the calculation that extends
q̂ from a 2-by-1 to a 3-by-2 guess, nor why it reduces the error bound.
The proof that the calculation even has the desired effect is left to exercises.
The solutions to those exercises provided at the back of the book are entirely
calculations, still with no explanation as to what is going on or how you would
arrive at the idea of doing those exact calculations. Nowhere is it mentioned
that this test extends the 2-by-1 guess into a 3-by-2 guess. The proof of the
Good Guess Guarantee is only for the 2-by-1 guess and argues by contradiction,
making it difficult to understand how modifications like adding another digit
or adjusting the quotient range affects the overall bound.

All that said, Knuth remains the canonical reference. It is dense but packed
full of information and references, and the proofs are simpler than many other
presentations. The proofs above are reworkings of Knuth's to remove the
arguments by contradiction and add explanations or steps that Knuth omitted.
But beware of errors in older printings. Take the published errata with you.

Brinch Hansen's “Multiple-length Division Revisited: a Tour of the Minefield”
starts with a blunt critique of Knuth's presentation (among others) and then
presents a more detailed and easier to follow treatment of long division,
including an implementation in Pascal. But the algorithm and implementation
work entirely in terms of 3-by-2 division, which is much less useful on modern
hardware than an algorithm using 2-by-1 division. The proofs are a bit too
focused on digit counting and seem needlessly complex, especially compared to
the ones given above.

Burnikel and Ziegler's “Fast Recursive Division” introduced the key insight of
implementing division by an n-digit divisor using recursive calls to division
by an n/2-digit divisor, relying on Karatsuba multiplication to yield a
sub-quadratic run time. However, the presentation decisions are made almost
entirely for the purpose of simplifying the run-time analysis, rather than
simplifying the presentation. Instead of a single algorithm that loops over
quotient digits, the paper presents two mutually-recursive algorithms, for
2n-by-n and 3n-by-2n. The paper also does not present any general (n+m)-by-n
algorithm.

The proofs in the paper are remarkably complex, especially considering that
the algorithm is at its core just long division on wide digits, so that the
usual long division proofs apply essentially unaltered.
*/

// divRecursiveThreshold is the number of divisor digits
// at which point divRecursive is faster than divBasic.
static constexpr int64_t divRecursiveThreshold = 40; // see calibrate_test.go

// rem returns r such that r = u%v.
// It uses z as the storage for r.
bool nat_rem(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &r) {
	PackedInt64Array q;
	return nat_div(u, v, q, r);
}

// div returns q, r such that q = ⌊u/v⌋ and r = u%v = u - q·v.
// It uses z and z2 as the storage for q and r.
// The caller may pass stk == nil to request that div obtain and release one itself.
bool nat_div(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &q, PackedInt64Array &r) {
	ERR_FAIL_COND_V_MSG(v.is_empty(), false, "division by zero");

	if (v.size() == 1) {
		// Short division: long optimized for a single-word divisor.
		// In that case, the 2-by-1 guess is all we need at each step.
		uint64_t r2 = nat_divW(u, uint64_t(v[0]), q);
		nat_setWord(r, r2);
		return true;
	}

	if (nat_cmp(u, v) < 0) {
		q.clear();
		nat_set(r, u);
		return true;
	}

	nat_divLarge(u, v, q, r);
	return true;
}

// divW returns q, r such that q = ⌊x/y⌋ and r = x%y = x - q·y.
// It uses z as the storage for q.
// Note that y is a single digit (Word), not a big number.
uint64_t nat_divW(PackedInt64Array x, uint64_t y, PackedInt64Array &q) {
	const int64_t m = x.size();
	CRASH_COND_MSG(y == 0, "division by zero");

	if (y == 1) {
		nat_set(q, x); // result is x
		return 0;
	}

	if (m == 0) {
		q.clear(); // result is 0
		return 0;
	}

	// m > 0
	q.resize(m);
	const uint64_t r = nat_divWVW(q, 0, x, y);
	nat_norm(q);

	return r;
}

// modW returns x % d.
uint64_t nat_modW(PackedInt64Array x, uint64_t d) {
	// TODO(agl): we don't actually need to store the q value.
	PackedInt64Array q;
	q.resize(x.size());
	return nat_divWVW(q, 0, x, d);
}

// divWVW overwrites z with ⌊x/y⌋, returning the remainder r.
// The caller must ensure that len(z) = len(x).
uint64_t nat_divWVW(PackedInt64Array &z, uint64_t xn, PackedInt64Array x, uint64_t y) {
	uint64_t r = xn;

	if (x.size() == 1) {
		uint64_t qq, rr;
		nat_div64(r, x[0], y, qq, rr);
		z[0] = qq;

		return rr;
	}

	const uint64_t rec = nat_reciprocalWord(y);
	for (int64_t i = z.size() - 1; i >= 0; i--) {
		uint64_t zi;
		nat_divWW(r, x[i], y, rec, zi, r);
		z[i] = zi;
	}

	return r;
}

// div returns q, r such that q = ⌊uIn/vIn⌋ and r = uIn%vIn = uIn - q·vIn.
// It uses z and u as the storage for q and r.
// The caller must ensure that len(vIn) ≥ 2 (use divW otherwise)
// and that len(uIn) ≥ len(vIn) (the answer is 0, uIn otherwise).
void nat_divLarge(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &q, PackedInt64Array &r) {
	const int64_t n = v.size();
	const int64_t m = u.size() - n;

	// Scale the inputs so vIn's top bit is 1 (see “Scaling Inputs” above).
	// vIn is treated as a read-only input (it may be in use by another
	// goroutine), so we must make a copy.
	// uIn is copied to u.
	const uint64_t shift = nat_nlz(v[n - 1]);
	PackedInt64Array v2;
	r.resize(u.size() + 1);
	if (shift == 0) {
		v2 = v;
		nat_copy(r, 0, u, 0, u.size());
		r[u.size()] = 0;
	} else {
		v2.resize(n);
		nat_lshVU(v2, 0, v, 0, shift, n);
		r[u.size()] = nat_lshVU(r, 0, u, 0, shift, u.size());
	}

	q.resize(m + 1);

	// Use basic or recursive long division depending on size.
	if (n < divRecursiveThreshold) {
		nat_divBasic(q, r, v2);
	} else {
		nat_divRecursive(q, r, v2);
	}

	nat_norm(q);

	// Undo scaling of remainder.
	if (shift != 0) {
		nat_rshVU(r, 0, r, 0, shift, n);
	}

	nat_norm(r);
}

// divBasic implements long division as described above.
// It overwrites q with ⌊u/v⌋ and overwrites u with the remainder r.
// q must be large enough to hold ⌊u/v⌋.
void nat_divBasic(PackedInt64Array &q, PackedInt64Array &u, PackedInt64Array v) {
	const int64_t n = v.size();
	const int64_t m = u.size() - n;

	PackedInt64Array qhatv;
	qhatv.resize(n + 1);

	// Set up for divWW below, precomputing reciprocal argument.
	uint64_t vn1 = v[n - 1];
	uint64_t rec = nat_reciprocalWord(vn1);

	// Invent a leading 0 for u, for the first iteration.
	// Invariant: ujn == u[j+n] in each iteration.
	uint64_t ujn = 0;

	// Compute each digit of quotient.
	for (int64_t j = m; j >= 0; j--) {
		// Compute the 2-by-1 guess q̂.
		uint64_t qhat = ~uint64_t(0);

		// ujn ≤ vn1, or else q̂ would be more than one digit.
		// For ujn == vn1, we set q̂ to the max digit M above.
		// Otherwise, we compute the 2-by-1 guess.
		if (ujn != vn1) {
			uint64_t rhat = 0;
			nat_divWW(ujn, u[j + n - 1], vn1, rec, qhat, rhat);

			// Refine q̂ to a 3-by-2 guess. See “Refining Guesses” above.
			uint64_t vn2 = v[n - 2];
			uint64_t x1, x0;
			nat_mulWW(qhat, vn2, x1, x0);
			uint64_t ujn2 = u[j + n - 2];
			while (nat_greaterThan(x1, x0, rhat, ujn2)) { // x1x2 > r̂ u[j+n-2]
				qhat--;
				const uint64_t prevRhat = rhat;
				rhat += vn1;
				// If r̂  overflows, then
				// r̂ u[j+n-2]v[n-1] is now definitely > x1 x2.
				if (rhat < prevRhat) {
					break;
				}
				// TODO(rsc): No need for a full mulWW.
				// x2 += vn2; if x2 overflows, x1++
				nat_mulWW(qhat, vn2, x1, x0);
			}
		}

		// Compute q̂·v.
		qhatv[n] = nat_mulAddVWW(qhatv, 0, v, 0, qhat, 0, n);
		int64_t qhl = qhatv.size();
		if (j + qhl > u.size() && qhatv[n] == 0) {
			qhl--;
		}

		// Subtract q̂·v from the current section of u.
		// If it underflows, q̂·v > u, which we fix up
		// by decrementing q̂ and adding v back.
		uint64_t c = nat_subVV(u, j, u, j, qhatv, 0, qhl);
		if (c != 0) {
			c = nat_addVV(u, j, u, j, v, 0, n);

			// If n == qhl, the carry from subVV and the carry from addVV
			// cancel out and don't affect u[j+n].
			if (n < qhl) {
				u[j + n] += c;
			}

			qhat--;
		}

		ujn = u[j + n - 1];

		// Save quotient digit.
		// Caller may know the top digit is zero and not leave room for it.
		if (j == m && m == q.size() && qhat == 0) {
			continue;
		}

		q[j] = qhat;
	}
}

// greaterThan reports whether the two digit numbers x1 x2 > y1 y2.
// TODO(rsc): In contradiction to most of this file, x1 is the high
// digit and x2 is the low digit. This should be fixed.
bool nat_greaterThan(uint64_t x1, uint64_t x0, uint64_t y1, uint64_t y0) {
	return x1 > y1 || (x1 == y1 && x0 > y0);
}

// divRecursive implements recursive division as described above.
// It overwrites z with ⌊u/v⌋ and overwrites u with the remainder r.
// z must be large enough to hold ⌊u/v⌋.
// This function is just for allocating and freeing temporaries
// around divRecursiveStep, the real implementation.
void nat_divRecursive(PackedInt64Array &q, PackedInt64Array &u, PackedInt64Array v) {
	q.fill(0);
	u = nat_divRecursiveStep(q, u, v, 0);
}

// divRecursiveStep is the actual implementation of recursive division.
// It adds ⌊u/v⌋ to z and overwrites u with the remainder r.
// z must be large enough to hold ⌊u/v⌋.
// It uses temps[depth] (allocating if needed) as a temporary live across
// the recursive call. It also uses tmp, but not live across the recursion.
PackedInt64Array nat_divRecursiveStep(PackedInt64Array &q, PackedInt64Array u, PackedInt64Array v, int64_t depth) {
	// u is a subsection of the original and may have leading zeros.
	// TODO(rsc): The v = v.norm() is useless and should be removed.
	// We know (and require) that v's top digit is ≥ B/2.
	nat_norm(u);
	nat_norm(v);
	if (u.is_empty()) {
		q.fill(0);
		return u;
	}

	// Fall back to basic division if the problem is now small enough.
	const int64_t n = v.size();
	if (n < divRecursiveThreshold) {
		nat_divBasic(q, u, v);
		return u;
	}

	// Nothing to do if u is shorter than v (implies u < v).
	const int64_t m = u.size() - n;
	if (m < 0) {
		return u;
	}

	// We consider B digits in a row as a single wide digit.
	// (See “Recursive Division” above.)
	//
	// TODO(rsc): rename B to Wide, to avoid confusion with _B,
	// which is something entirely different.
	// TODO(rsc): Look into whether using ⌈n/2⌉ is better than ⌊n/2⌋.
	const int64_t B = n / 2;

	// Allocate a nat for qhat below.
	PackedInt64Array qhat0;
	qhat0.resize(B + 1);

	// Compute each wide digit of the quotient.
	//
	// TODO(rsc): Change the loop to be
	//	for j := (m+B-1)/B*B; j > 0; j -= B {
	// which will make the final step a regular step, letting us
	// delete what amounts to an extra copy of the loop body below.
	int64_t j = m;
	while (j > B) {
		// Divide u[j-B:j+n] (3 wide digits) by v (2 wide digits).
		// First make the 2-by-1-wide-digit guess using a recursive call.
		// Then extend the guess to the full 3-by-2 (see “Refining Guesses”).
		//
		// For the 2-by-1-wide-digit guess, instead of doing 2B-by-B-digit,
		// we use a (2B+1)-by-(B+1) digit, which handles the possibility that
		// the result has an extra leading 1 digit as well as guaranteeing
		// that the computed q̂ will be off by at most 1 instead of 2.

		// s is the number of digits to drop from the 3B- and 2B-digit chunks.
		// We drop B-1 to be left with 2B+1 and B+1.
		const int64_t s = B - 1;

		// uu is the up-to-3B-digit section of u we are working on.
		const int64_t uu0 = j - B; // uu := u[j-B:]

		// Compute the 2-by-1 guess q̂, leaving r̂ in uu[s:B+n].
		PackedInt64Array qhat = qhat0;
		qhat.fill(0);
		const PackedInt64Array uout = nat_divRecursiveStep(qhat, u.slice(uu0 + s, uu0 + B + n), v.slice(s), depth + 1);
		for (int64_t i = 0; i < uout.size(); i++) {
			u[uu0 + s + i] = uout[i];
		}
		nat_norm(qhat);

		// Extend to a 3-by-2 quotient and remainder.
		// Because divRecursiveStep overwrote the top part of uu with
		// the remainder r̂, the full uu already contains the equivalent
		// of r̂·B + uₙ₋₂ from the “Refining Guesses” discussion.
		// Subtracting q̂·vₙ₋₂ from it will compute the full-length remainder.
		// If that subtraction underflows, q̂·v > u, which we fix up
		// by decrementing q̂ and adding v back, same as in long division.

		// TODO(rsc): Instead of subtract and fix-up, this code is computing
		// q̂·vₙ₋₂ and decrementing q̂ until that product is ≤ u.
		// But we can do the subtraction directly, as in the comment above
		// and in long division, because we know that q̂ is wrong by at most one.
		PackedInt64Array qhatv;
		nat_mul(qhatv, qhat, v.slice(0, s));
		for (int64_t i = 0; i < 2; i++) {
			PackedInt64Array uunorm = u.slice(uu0);
			nat_norm(uunorm);
			if (nat_cmp(qhatv, uunorm) <= 0) {
				break;
			}

			nat_subVW(qhat, 0, qhat, 0, 1, qhat.size());
			uint64_t c = nat_subVV(qhatv, 0, qhatv, 0, v, 0, s);
			if (qhatv.size() > s) {
				nat_subVW(qhatv, s, qhatv, s, c, qhatv.size() - s);
			}

			nat_addTo(u, uu0 + s, v.slice(s));
		}

		PackedInt64Array uunorm = u.slice(uu0);
		nat_norm(uunorm);
		CRASH_COND(nat_cmp(qhatv, uunorm) > 0);

		uint64_t c = nat_subVV(u, uu0, u, uu0, qhatv, 0, qhatv.size());
		if (c > 0) {
			nat_subVW(u, uu0 + qhatv.size(), u, uu0 + qhatv.size(), c, u.size() - uu0 - qhatv.size());
		}

		nat_addTo(q, j - B, qhat);

		j -= B;
	}

	// TODO(rsc): Rewrite loop as described above and delete all this code.

	// Now u < (v<<B), compute lower bits in the same way.
	// Choose shift = B-1 again.
	const int64_t s = B - 1;
	PackedInt64Array qhat = qhat0;
	qhat.fill(0);
	const PackedInt64Array uout = nat_divRecursiveStep(qhat, u.slice(s), v.slice(s), depth + 1);
	for (int64_t i = 0; i < uout.size(); i++) {
		u[s + i] = uout[i];
	}
	nat_norm(qhat);

	PackedInt64Array qhatv;
	nat_mul(qhatv, qhat, v.slice(0, s));

	// Set the correct remainder as before.
	for (int64_t i = 0; i < 2; i++) {
		PackedInt64Array unorm = u;
		nat_norm(unorm);

		if (nat_cmp(qhatv, unorm) > 0) {
			nat_subVW(qhat, 0, qhat, 0, 1, qhat.size());

			uint64_t c = nat_subVV(qhatv, 0, qhatv, 0, v, 0, s);
			if (qhatv.size() > s) {
				nat_subVW(qhatv, s, qhatv, s, c, qhatv.size() - s);
			}

			nat_addTo(u, s, v.slice(s));
		}
	}

	PackedInt64Array unorm = u;
	nat_norm(unorm);
	CRASH_COND(nat_cmp(qhatv, unorm) > 0);

	uint64_t c = nat_subVV(u, 0, u, 0, qhatv, 0, qhatv.size());
	if (c > 0) {
		c = nat_subVW(u, qhatv.size(), u, qhatv.size(), c, u.size() - qhatv.size());
	}
	CRASH_COND(c > 0);

	// Done!
	nat_norm(qhat);
	nat_addTo(q, 0, qhat);

	return u;
}
