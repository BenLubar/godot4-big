#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/templates/span.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

#include <functional>

enum BigAccuracy {
	ACC_BELOW = -1,
	ACC_EXACT = 0,
	ACC_ABOVE = 1,
};

typedef uint64_t BigWord;

struct BigDivisor;

struct BigNat {
	godot::PackedInt64Array array;

	void norm();
	void setUint64(uint64_t p_x);
	void set(BigNat p_x);
	void add(BigNat p_x, BigNat p_y);
	void sub(BigNat p_x, BigNat p_y);
	int cmp(BigNat p_y) const;
	int cmpnorm(BigNat p_y) const;
	void montgomery(BigNat p_x, BigNat p_y, BigNat p_m, BigWord p_k, int64_t p_n);
	void mulRange(uint64_t p_x, uint64_t p_y);

	int64_t bitLen() const;
	uint64_t trailingZeroBits() const;
	godot::Pair<uint64_t, bool> isPow2() const;
	void lsh(BigNat p_x, uint64_t p_s);
	void rsh(BigNat p_x, uint64_t p_s);
	void setBit(BigNat p_x, uint64_t p_i, uint64_t p_s);
	uint64_t bit(uint64_t p_i) const;
	uint64_t sticky(uint64_t p_i) const;
	void and_(BigNat p_x, BigNat p_y);
	void trunc(BigNat p_x, uint64_t p_n);
	void andNot(BigNat p_x, BigNat p_y);
	void or_(BigNat p_x, BigNat p_y);
	void xor_(BigNat p_x, BigNat p_y);
	void random(const std::function<uint32_t()> &p_rnd, BigNat p_limit, int64_t p_n);
	void expNN(BigNat p_x, BigNat p_y, BigNat p_m, bool p_slow);
	void expNNMontgomeryEven(BigNat p_x, BigNat p_y, BigNat p_m);
	void expNNWindowed(BigNat p_x, BigNat p_y, uint64_t p_log_m);
	void expNNMontgomery(BigNat p_x, BigNat p_y, BigNat p_m);
	int64_t bytes(godot::PackedByteArray &r_buf) const;
	void setBytes(const godot::PackedByteArray &p_buf);
	void sqrt(BigNat p_x);
	void subMod2N(BigNat p_x, BigNat p_y, uint64_t p_n);

	void rem(BigNat p_u, BigNat p_v);
	void div(BigNat &r_r, BigNat p_u, BigNat p_v);
	BigWord divW(BigNat p_x, BigWord p_y);
	BigWord modW(BigWord p_d) const;
	static BigWord divWVW(BigNat &p_z, BigWord p_xn, BigNat p_x, BigWord p_y);
	static void divWW(BigWord p_x1, BigWord p_x0, BigWord p_y, BigWord p_m, BigWord &r_q, BigWord &r_r);
	void divLarge(BigNat &r_r, BigNat p_u, BigNat p_v);
	void divBasic(BigNat &r_u, BigNat p_v);
	void divRecursive(BigNat &r_u, BigNat p_v);
	void divRecursiveStep(BigNat &r_u, BigNat p_v, int64_t p_depth);
	static void divWW_basic(BigWord p_hi, BigWord p_lo, BigWord p_y, BigWord &r_quo, BigWord &r_rem);
	static bool greaterThan(BigWord x1, BigWord x0, BigWord y1, BigWord y0);
	static BigWord reciprocalWord(BigWord d1);

	void mul(BigNat p_x, BigNat p_y);
	void sqr(BigNat p_x);
	void mulAddWW(BigNat p_x, BigWord p_y, BigWord p_r);
	static void addTo(BigNat &r_z, int64_t p_start, BigNat p_x);
	static void basicMul(BigNat &r_z, BigNat p_x, BigNat p_y);
	static void basicSqr(BigNat &r_z, BigNat p_x);
	static void karatsuba(BigNat &r_z, BigNat p_x, BigNat p_y);
	static void karatsubaSqr(BigNat &r_z, BigNat p_x);

	bool probablyPrimeMillerRabin(int64_t p_reps, bool force2) const;
	bool probablyPrimeLucas() const;

	godot::Error modInverse(BigNat g, BigNat n);

	void expWW(BigWord p_x, BigWord p_y);
	_FORCE_INLINE_ godot::String utoa(int64_t p_base) const { return itoa(false, p_base); }
	godot::String itoa(bool p_neg, int64_t p_base) const;
	void convertWords(uint8_t *r_s, int64_t r_s_size, BigWord p_b, int64_t p_ndigits, BigWord p_bb, const godot::Vector<BigDivisor> &p_table);

	uint32_t low32() const;
	uint64_t low64() const;
	int64_t fnorm();
	uint32_t msb32() const;
	uint64_t msb64() const;

	static void mulWW(BigWord p_x, BigWord p_y, BigWord &r_z1, BigWord &r_z0);
	static void mulAddWWW(BigWord p_x, BigWord p_y, BigWord p_c, BigWord &r_z1, BigWord &r_z0);
	static BigWord mulAddVWW(BigNat &r_z, BigNat p_x, BigWord p_y, BigWord p_r);
	static BigWord addMulVVWW(BigNat &r_z, BigNat p_x, BigNat p_y, BigWord p_m, BigWord p_a);
	static BigWord addVV(BigNat &r_z, BigNat p_x, BigNat p_y);
	static BigWord subVV(BigNat &r_z, BigNat p_x, BigNat p_y);
	static BigWord addVW(BigNat &r_z, BigNat p_x, BigWord p_y);
	static BigWord subVW(BigNat &r_z, BigNat p_x, BigWord p_y);
	static BigWord lshVU(BigNat &r_z, BigNat p_x, uint64_t p_s);
	static BigWord rshVU(BigNat &r_z, BigNat p_x, uint64_t p_s);

	BigWord &operator[](int64_t p_index) {
		return reinterpret_cast<BigWord &>(array[p_index]);
	}
	BigWord operator[](int64_t p_index) const {
		return BigWord(array[p_index]);
	}

	godot::PackedByteArray to_uvarint() const;
	uint64_t from_uvarint(const godot::Span<uint8_t> &p_bytes);

	static godot::Error scanSign(const godot::String &s, int64_t &off, bool &neg);
	static godot::Error scanExponent(const godot::String &s, int64_t &off, bool base2ok, bool sepOk, int64_t &exp, int64_t &base);
	static BigWord pow(BigWord x, int64_t n);
	godot::Error scan(const godot::String &s, int64_t &off, int64_t base, bool fracOk, int64_t &b, int64_t &count);
};

struct BigDivisor {
	BigNat bbb;          // divisor
	int64_t nbits = 0;   // bit length of divisor (discounting leading zeros) ~= log2(bbb)
	int64_t ndigits = 0; // digit length of divisor in terms of output base digits
};
