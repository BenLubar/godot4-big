#pragma once

#include "godot_big_naturals.h"

class BigInt : public godot::Resource {
	GDCLASS(BigInt, godot::Resource);

protected:
	static void _bind_methods();

public:
	bool _neg = false; // sign
	BigNat _abs; // absolute value of the integer
	void _set_neg(bool p_neg);
	bool _is_neg() const;
	void _set_abs(const godot::PackedInt64Array &p_abs);
	godot::PackedInt64Array _get_abs() const;

	godot::PackedByteArray to_uvarint() const;
	int64_t from_uvarint(const godot::PackedByteArray &p_bytes, int64_t p_offset = 0);
	godot::PackedByteArray to_svarint() const;
	int64_t from_svarint(const godot::PackedByteArray &p_bytes, int64_t p_offset = 0);

	int Sign() const;

	void SetInt64(int64_t p_x);
	void SetUint64(uint64_t p_x);
	static godot::Ref<BigInt> NewInt(int64_t p_x);
	void Set(const godot::Ref<BigInt> &p_x);

	void Abs(const godot::Ref<BigInt> &p_x);
	void Neg(const godot::Ref<BigInt> &p_x);
	void Add(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void Sub(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void Mul(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void MulRange(int64_t p_a, int64_t p_b);
	void Binomial(int64_t p_n, int64_t p_k);
	godot::Error Quo(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	godot::Error Rem(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	godot::Error QuoRem(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y, const godot::Ref<BigInt> &r_r);
	godot::Error Div(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	godot::Error Mod(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	godot::Error DivMod(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y, const godot::Ref<BigInt> &r_m);

	int Cmp(const godot::Ref<BigInt> &p_y) const;
	int CmpAbs(const godot::Ref<BigInt> &p_y) const;

	int64_t Int64() const;
	uint64_t Uint64() const;
	bool IsInt64() const;
	bool IsUint64() const;

	godot::Pair<double, BigAccuracy> Float64() const;
	_FORCE_INLINE_ double _Float64_bind() const { return Float64().first; }
	_FORCE_INLINE_ BigAccuracy _Float64Accuracy_bind() const { return Float64().second; }

	void SetBytes(const godot::PackedByteArray &p_bytes);
	godot::PackedByteArray Bytes() const;
	void FillBytes(godot::PackedByteArray &r_bytes) const; // not exposed to gdscript
	int64_t BitLen() const;
	uint64_t TrailingZeroBits() const;

	godot::Error Exp(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y, const godot::Ref<BigInt> &p_m = nullptr);
	godot::Error expSlow(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y, const godot::Ref<BigInt> &p_m);
	godot::Error _exp(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y, const godot::Ref<BigInt> &p_m, bool p_slow);
	void GCD(const godot::Ref<BigInt> &r_x, const godot::Ref<BigInt> &r_y, const godot::Ref<BigInt> &p_a, const godot::Ref<BigInt> &p_b);

	static void _lehmerSimulate(const godot::Ref<BigInt> &A, const godot::Ref<BigInt> &B, BigWord &u0, BigWord &u1, BigWord &v0, BigWord &v1, bool &even);
	static void _lehmerUpdate(const godot::Ref<BigInt> &A, const godot::Ref<BigInt> &B, const godot::Ref<BigInt> &q, const godot::Ref<BigInt> &r, BigWord u0, BigWord u1, BigWord v0, BigWord v1, bool even);
	void _mulW(const godot::Ref<BigInt> &x, bool neg, BigWord w);
	static void _euclidUpdate(godot::Ref<BigInt> &A, godot::Ref<BigInt> &B, godot::Ref<BigInt> &Ua, godot::Ref<BigInt> &Ub, const godot::Ref<BigInt> &q, godot::Ref<BigInt> &r, bool extended);
	void _lehmerGCD(const godot::Ref<BigInt> &r_x, const godot::Ref<BigInt> &r_y, const godot::Ref<BigInt> &p_a, const godot::Ref<BigInt> &p_b);

	void Rand(const std::function<uint32_t()> &p_rnd, const godot::Ref<BigInt> &p_n);
	void _Rand_bind(const godot::Callable &p_rnd, const godot::Ref<BigInt> &p_n);

	godot::Error ModInverse(const godot::Ref<BigInt> &p_g, const godot::Ref<BigInt> &p_n);
	static int Jacobi(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void _modSqrt3Mod4Prime(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_p);
	void _modSqrt5Mod8Prime(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_p);
	void _modSqrtTonelliShanks(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_p);
	godot::Error ModSqrt(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_p);

	void Lsh(const godot::Ref<BigInt> &p_x, uint64_t p_n);
	void Rsh(const godot::Ref<BigInt> &p_x, uint64_t p_n);
	uint64_t Bit(int64_t p_i) const;
	void SetBit(const godot::Ref<BigInt> &p_x, int64_t p_i, uint64_t p_b);
	void And(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void AndNot(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void Or(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void Xor(const godot::Ref<BigInt> &p_x, const godot::Ref<BigInt> &p_y);
	void Not(const godot::Ref<BigInt> &p_x);

	godot::Error Sqrt(const godot::Ref<BigInt> &p_x);

	static constexpr int64_t MAX_BASE = 10 + ('z' - 'a' + 1) + ('Z' - 'A' + 1);
	godot::Error _scan(const godot::String &s, int64_t &off, int64_t &base);
	godot::Error SetString(const godot::String &p_s, int64_t p_base = 0);
	godot::String String(int64_t p_base = 10) const;
	_FORCE_INLINE_ godot::String _to_string() const { return String(); }

	bool ProbablyPrime(int64_t n) const;
};
