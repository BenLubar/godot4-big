#pragma once

#include "godot_big_int.h"

class BigRat : public GODOT_BIG_BASE_CLASS {
	GDCLASS(BigRat, GODOT_BIG_BASE_CLASS);

protected:
	static void _bind_methods();

private:
	godot::Ref<BigInt> _a = memnew(BigInt);
	godot::Ref<BigInt> _b = memnew(BigInt);
	friend class BigFloat;

public:
	static godot::Ref<BigRat> make(int64_t p_a, int64_t p_b);

	// raw getters/setters for serialization
	inline bool _is_neg() const { return _a->_neg; }
	inline void _set_neg(bool p_neg) { _a->_neg = p_neg; }
	inline godot::PackedInt64Array _get_a() const { return _a->_abs; }
	inline void _set_a(const godot::PackedInt64Array &p_a) { _a->_abs = p_a; }
	inline godot::PackedInt64Array _get_b() const { return _b->_abs; }
	inline void _set_b(const godot::PackedInt64Array &p_b) { _b->_abs = p_b; }

	godot::Ref<BigRat> SetFloat64(double f);
	float Float32() const;
	bool Float32Exact() const;
	double Float64() const;
	bool Float64Exact() const;
	godot::Ref<BigRat> SetFrac(const godot::Ref<BigInt> &a, const godot::Ref<BigInt> &b);
	godot::Ref<BigRat> SetFrac64(int64_t a, int64_t b);
	godot::Ref<BigRat> SetInt(const godot::Ref<BigInt> &x);
	godot::Ref<BigRat> SetInt64(int64_t x);
	godot::Ref<BigRat> SetUint64(uint64_t x);
	godot::Ref<BigRat> Set(const godot::Ref<BigRat> &x);
	godot::Ref<BigRat> Abs(const godot::Ref<BigRat> &x);
	godot::Ref<BigRat> Neg(const godot::Ref<BigRat> &x);
	godot::Ref<BigRat> Inv(const godot::Ref<BigRat> &x);
	int Sign() const;
	bool IsInt() const;
	godot::Ref<BigInt> Num() const;
	godot::Ref<BigInt> Denom() const;
	godot::Ref<BigRat> norm();
	int Cmp(const godot::Ref<BigRat> &y) const;
	godot::Ref<BigRat> Add(const godot::Ref<BigRat> &x, const godot::Ref<BigRat> &y);
	godot::Ref<BigRat> Sub(const godot::Ref<BigRat> &x, const godot::Ref<BigRat> &y);
	godot::Ref<BigRat> Mul(const godot::Ref<BigRat> &x, const godot::Ref<BigRat> &y);
	godot::Ref<BigRat> Quo(const godot::Ref<BigRat> &x, const godot::Ref<BigRat> &y);

	godot::Ref<BigRat> SetString(const godot::String &s);
	godot::String String() const;
	inline godot::String _to_string() const { return String(); }
	godot::String RatString() const;
	godot::String FloatString(int64_t prec) const;
	void FloatPrec(int64_t &n, bool &exact) const;
};
