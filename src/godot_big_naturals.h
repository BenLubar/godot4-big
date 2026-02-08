#pragma once

#include <bit>
#include <cstdint>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

using namespace godot;

// allocated and deallocated by godot_big_register_types.cpp
class BigInt;
class BigRat;
class BigFloat;
struct BigDivisor;
extern PackedInt64Array *natOne;
extern PackedInt64Array *natTwo;
extern PackedInt64Array *natThree;
extern PackedInt64Array *natFive;
extern PackedInt64Array *natTen;
extern Ref<BigInt> *intOne;
extern Ref<BigFloat> *floatThree;
extern Vector<BigDivisor> *cacheBase10;
extern std::mutex cacheBase10Mutex;

// arith.go
void nat_mulWW(uint64_t x, uint64_t y, uint64_t &z1, uint64_t &z0);
void nat_mulAddWWW(uint64_t x, uint64_t y, uint64_t c, uint64_t &z1, uint64_t &z0);
uint64_t nat_nlz(uint64_t x);
uint64_t nat_addWW(uint64_t x, uint64_t y, uint64_t &carry); // bits.Add64 in Go
uint64_t nat_addVV(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, int64_t n);
uint64_t nat_subWW(uint64_t x, uint64_t y, uint64_t &borrow); // bits.Sub64 in Go
uint64_t nat_subVV(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, int64_t n);
uint64_t nat_addVW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, int64_t n);
uint64_t nat_subVW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, int64_t n);
uint64_t nat_lshVU(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t s, int64_t n);
uint64_t nat_rshVU(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t s, int64_t n);
uint64_t nat_mulAddVWW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, uint64_t y, uint64_t r, int64_t n);
uint64_t nat_addMulVVWW(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, PackedInt64Array y, int64_t yoff, uint64_t m, uint64_t a, int64_t n);
void nat_divWW(uint64_t x1, uint64_t x0, uint64_t y, uint64_t m, uint64_t &q, uint64_t &r);
void nat_div64(uint64_t hi, uint64_t lo, uint64_t y, uint64_t &quo, uint64_t &rem);
uint64_t nat_reciprocalWord(uint64_t d1);

// nat.go
void nat_norm(PackedInt64Array &z);
void nat_make(PackedInt64Array &z, int64_t n);
void nat_setWord(PackedInt64Array &z, uint64_t x);
void nat_setUint64(PackedInt64Array &z, uint64_t x);
void nat_set(PackedInt64Array &z, PackedInt64Array x);
void nat_copy(PackedInt64Array &z, int64_t zoff, PackedInt64Array x, int64_t xoff, int64_t n); // builtin copy function in Go
void nat_clear(PackedInt64Array &z, int64_t zoff, int64_t n); // builtin clear function in Go
void nat_add(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_sub(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
int nat_cmp(PackedInt64Array x, PackedInt64Array y);
void nat_montgomery(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m, uint64_t k, uint64_t n);
void nat_addTo(PackedInt64Array &z, int64_t zoff, PackedInt64Array x);
void nat_mulRange(PackedInt64Array &z, uint64_t a, uint64_t b);
int64_t nat_bitLen(PackedInt64Array x);
uint64_t nat_trailingZeroBits(PackedInt64Array x);
bool nat_isPow2(PackedInt64Array x, uint64_t &i);
void nat_lsh(PackedInt64Array &z, PackedInt64Array x, uint64_t s);
void nat_rsh(PackedInt64Array &z, PackedInt64Array x, uint64_t s);
void nat_setBit(PackedInt64Array &z, PackedInt64Array x, uint64_t i, uint64_t b);
uint64_t nat_bit(PackedInt64Array x, uint64_t i);
uint64_t nat_sticky(PackedInt64Array x, uint64_t i);
void nat_and(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_trunc(PackedInt64Array &z, PackedInt64Array x, uint64_t n);
void nat_andNot(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_or(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_xor(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_random(PackedInt64Array &z, const Callable &rand, PackedInt64Array limit, int64_t n);
void nat_expNN(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m, bool slow);
void nat_expNNMontgomeryEven(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m);
void nat_expNNWindowed(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, uint64_t logM);
void nat_expNNMontgomery(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, PackedInt64Array m);
int64_t nat_bytes(PackedInt64Array z, PackedByteArray &buf);
uint64_t nat_bigEndianWord(PackedByteArray buf, int64_t i);
void nat_setBytes(PackedInt64Array &z, PackedByteArray buf);
void nat_sqrt(PackedInt64Array &z, PackedInt64Array x);
void nat_subMod2N(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y, uint64_t n);

// natmul.go
void nat_mul(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_sqr(PackedInt64Array &z, PackedInt64Array x);
void nat_basicSqr(PackedInt64Array &z, PackedInt64Array x);
void nat_mulAddWW(PackedInt64Array &z, PackedInt64Array x, uint64_t y, uint64_t r);
void nat_basicMul(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_karatsuba(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);
void nat_karatsubaSqr(PackedInt64Array &z, PackedInt64Array x);

// natdiv.go
bool nat_rem(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &r);
bool nat_div(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &q, PackedInt64Array &r);
uint64_t nat_divW(PackedInt64Array x, uint64_t y, PackedInt64Array &q);
uint64_t nat_modW(PackedInt64Array x, uint64_t d);
uint64_t nat_divWVW(PackedInt64Array &z, uint64_t xn, PackedInt64Array x, uint64_t y);
void nat_divLarge(PackedInt64Array u, PackedInt64Array v, PackedInt64Array &q, PackedInt64Array &r);
void nat_divBasic(PackedInt64Array &q, PackedInt64Array &u, PackedInt64Array v);
bool nat_greaterThan(uint64_t x1, uint64_t x0, uint64_t y1, uint64_t y0);
void nat_divRecursive(PackedInt64Array &q, PackedInt64Array &u, PackedInt64Array v);
PackedInt64Array nat_divRecursiveStep(PackedInt64Array &q, PackedInt64Array u, PackedInt64Array v, int64_t depth);

// natconv.go
bool nat_scan(PackedInt64Array &z, const PackedByteArray &buf, int64_t &i, int64_t &base, int64_t &count, bool fracOk);
PackedByteArray nat_utoa(PackedInt64Array x, int64_t base);
PackedByteArray nat_itoa(PackedInt64Array x, bool neg, int64_t base);
void nat_expWW(PackedInt64Array &z, uint64_t x, uint64_t y);
struct BigDivisor {
	PackedInt64Array bbb; // divisor
	int64_t nbits = 0;    // bit length of divisor (discounting leading zeros) ~= log2(bbb)
	int64_t ndigits = 0;  // digit length of divisor in terms of output base digits
};
void nat_maxPow(uint64_t b, uint64_t &p, int64_t &n);
uint64_t nat_pow(uint64_t x, int64_t n);
Vector<BigDivisor> nat_divisors(int64_t m, uint64_t b, int64_t ndigits, uint64_t bb);
void nat_convertWords(PackedInt64Array q, PackedByteArray &s, int64_t soff, int64_t n, uint64_t b, int64_t ndigits, uint64_t bb, const Vector<BigDivisor> &table, int64_t tablesize);

// int.go
uint32_t nat_low32(PackedInt64Array x);
uint64_t nat_low64(PackedInt64Array x);
void nat_modInverse(PackedInt64Array &z, PackedInt64Array g, PackedInt64Array n);

// intconv.go
bool nat_scanSign(const PackedByteArray &buf, int64_t &i, bool &neg);

// prime.go
bool nat_probablyPrimeMillerRabin(PackedInt64Array n, int64_t reps, bool force2);
bool nat_probablyPrimeLucas(PackedInt64Array n);

// rat.go
void nat_quotToFloat32(PackedInt64Array a, PackedInt64Array b, float &f, bool &exact);
void nat_quotToFloat64(PackedInt64Array a, PackedInt64Array b, double &f, bool &exact);
void nat_mulDenom(PackedInt64Array &z, PackedInt64Array x, PackedInt64Array y);

// ratconv.go
bool nat_scanExponent(const PackedByteArray &buf, int64_t &i, bool base2ok, bool sepOk, int64_t &exp, int64_t &ebase);

// float.go
uint32_t nat_msb32(PackedInt64Array x);
uint64_t nat_msb64(PackedInt64Array x);
int64_t nat_fnorm(PackedInt64Array &m);

// decimal.go
struct BigDecimal {
	PackedByteArray mant;
	int64_t exp;

	char at(int64_t i) const;
	void init(PackedInt64Array m, int64_t shift);
	void rsh(uint64_t s);
	bool shouldRoundUp(int64_t n) const;
	void round(int64_t n);
	void roundDown(int64_t n);
	void roundUp(int64_t n);
	void roundShortest(const Ref<BigFloat> &x);
	void trim();
};
