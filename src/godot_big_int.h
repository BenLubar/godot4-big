#pragma once

#include "godot_big_shared.h"

class BigInt : public GODOT_BIG_BASE_CLASS {
	GDCLASS(BigInt, GODOT_BIG_BASE_CLASS);

protected:
	static void _bind_methods();

private:
	bool _neg = false; // sign
	godot::PackedInt64Array _abs; // absolute value of the integer
	friend class BigRat;
	friend class BigFloat;

public:
	static constexpr int64_t MAX_BASE = 10 + ('z' - 'a' + 1) + ('Z' - 'A' + 1);

	static godot::Ref<BigInt> make(int64_t p_x);

	// raw getters/setters for serialization
	inline bool _is_neg() const { return _neg; }
	inline void _set_neg(bool p_neg) { _neg = p_neg; }
	inline godot::PackedInt64Array _get_abs() const { return _abs; }
	inline void _set_abs(const godot::PackedInt64Array &p_abs) { _abs = p_abs; }

	int Sign() const;
	godot::Ref<BigInt> SetInt64(int64_t x);
	godot::Ref<BigInt> SetUint64(uint64_t x);
	godot::Ref<BigInt> Set(const godot::Ref<BigInt> &x);
	godot::Ref<BigInt> Abs(const godot::Ref<BigInt> &x);
	godot::Ref<BigInt> Neg(const godot::Ref<BigInt> &x);
	godot::Ref<BigInt> Add(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Sub(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Mul(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> MulRange(int64_t a, int64_t b);
	godot::Ref<BigInt> Binomial(int64_t n, int64_t k);
	godot::Ref<BigInt> Quo(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Rem(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> QuoRem(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &r);
	godot::Ref<BigInt> Div(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Mod(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> DivMod(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &m);
	int Cmp(const godot::Ref<BigInt> &y) const;
	int CmpAbs(const godot::Ref<BigInt> &y) const;
	int64_t Int64() const;
	uint64_t Uint64() const;
	bool IsInt64() const;
	bool IsUint64() const;
	double Float64() const;
	BigAccuracy Float64Accuracy() const;
	godot::Ref<BigInt> SetString(const godot::String &s, int64_t base);
	godot::Ref<BigInt> SetBytes(const godot::PackedByteArray &buf);
	godot::PackedByteArray Bytes() const;
	int64_t BitLen() const;
	uint64_t TrailingZeroBits() const;
	godot::Ref<BigInt> Exp(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &m);
	godot::Ref<BigInt> expSlow(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &m);
	godot::Ref<BigInt> exp(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &m, bool slow);
	godot::Ref<BigInt> GCD(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &a, const godot::Ref<BigInt> &b);
	static void lehmerSimulate(const godot::Ref<BigInt> &A, const godot::Ref<BigInt>&B, uint64_t &u0, uint64_t &u1, uint64_t &v0, uint64_t &v1, bool &even);
	static void lehmerUpdate(const godot::Ref<BigInt> &A, const godot::Ref<BigInt> &B, const godot::Ref<BigInt> &q, const godot::Ref<BigInt> &r, uint64_t u0, uint64_t u1, uint64_t v0, uint64_t v1, bool even);
	void mulW(const godot::Ref<BigInt> &x, bool neg, uint64_t w);
	static void euclidUpdate(godot::Ref<BigInt> &A, godot::Ref<BigInt> &B, godot::Ref<BigInt> &Ua, godot::Ref<BigInt> &Ub, const godot::Ref<BigInt> &q, godot::Ref<BigInt> &r, bool extended);
	godot::Ref<BigInt> lehmerGCD(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y, const godot::Ref<BigInt> &a, const godot::Ref<BigInt> &b);
	godot::Ref<BigInt> Rand(const godot::Callable &rnd, const godot::Ref<BigInt> &n);
	godot::Ref<BigInt> ModInverse(const godot::Ref<BigInt> &g, const godot::Ref<BigInt> &n);
	static int Jacobi(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> modSqrt3Mod4Prime(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &p);
	godot::Ref<BigInt> modSqrt5Mod8Prime(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &p);
	godot::Ref<BigInt> modSqrtTonelliShanks(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &p);
	godot::Ref<BigInt> ModSqrt(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &p);
	godot::Ref<BigInt> Lsh(const godot::Ref<BigInt> &x, uint64_t n);
	godot::Ref<BigInt> Rsh(const godot::Ref<BigInt> &x, uint64_t n);
	uint64_t Bit(int64_t i) const;
	godot::Ref<BigInt> SetBit(const godot::Ref<BigInt> &x, int64_t i, uint64_t b);
	godot::Ref<BigInt> And(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> AndNot(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Or(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Xor(const godot::Ref<BigInt> &x, const godot::Ref<BigInt> &y);
	godot::Ref<BigInt> Not(const godot::Ref<BigInt> &x);
	godot::Ref<BigInt> Sqrt(const godot::Ref<BigInt> &x);

	bool ProbablyPrime(int64_t n) const;
	godot::String Text(int64_t base) const;
	inline godot::String _to_string() const { return Text(10); }
	void scaleDenom(const godot::Ref<BigInt> &x, godot::PackedInt64Array f);

	godot::Ref<BigInt> scan(const godot::PackedByteArray &buf, int64_t &i, int64_t &base);
};
