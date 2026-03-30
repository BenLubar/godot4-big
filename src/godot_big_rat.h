#pragma once

#include "godot_big_naturals.h"

#include <godot_cpp/classes/resource.hpp>

class BigInt;

class BigRat : public godot::Resource {
	GDCLASS(BigRat, godot::Resource);

protected:
	static void _bind_methods();

public:
	bool _neg = false;
	BigNat _a, _b{{1}};
	void _set_neg(bool p_neg);
	bool _is_neg() const;
	void _set_a(const godot::PackedInt64Array &p_a);
	godot::PackedInt64Array _get_a() const;
	void _set_b(const godot::PackedInt64Array &p_b);
	godot::PackedInt64Array _get_b() const;

	godot::PackedByteArray to_bytes() const;
	int64_t from_bytes(const godot::PackedByteArray &p_bytes, int64_t p_offset = 0);

	static godot::Ref<BigRat> NewRat(int64_t p_a, int64_t p_b);

	godot::Error SetFloat64(double p_f);
	godot::Error SetFrac(const godot::Ref<BigInt> &p_a, const godot::Ref<BigInt> &p_b);
	godot::Error SetFrac64(int64_t p_a, int64_t p_b);
	void SetInt(const godot::Ref<BigInt> &p_x);
	void SetInt64(int64_t p_x);
	void SetUint64(uint64_t p_x);
	void Set(const godot::Ref<BigRat> &p_x);

	godot::Pair<float, bool> Float32() const;
	_FORCE_INLINE_ float _Float32_bind() const { return Float32().first; }
	_FORCE_INLINE_ bool _Float32Exact_bind() const { return Float32().second; }
	godot::Pair<double, bool> Float64() const;
	_FORCE_INLINE_ double _Float64_bind() const { return Float64().first; }
	_FORCE_INLINE_ bool _Float64Exact_bind() const { return Float64().second; }

	void Abs(const godot::Ref<BigRat> &p_x);
	void Neg(const godot::Ref<BigRat> &p_x);
	godot::Error Inv(const godot::Ref<BigRat> &p_x);
	int Sign() const;
	bool IsInt() const;

	void Num(const godot::Ref<BigInt> &r_a) const;
	void Denom(const godot::Ref<BigInt> &r_b) const;
	void _norm();
	int Cmp(const godot::Ref<BigRat> &p_y) const;

	void Add(const godot::Ref<BigRat> &p_x, const godot::Ref<BigRat> &p_y);
	void Sub(const godot::Ref<BigRat> &p_x, const godot::Ref<BigRat> &p_y);
	void Mul(const godot::Ref<BigRat> &p_x, const godot::Ref<BigRat> &p_y);
	godot::Error Quo(const godot::Ref<BigRat> &p_x, const godot::Ref<BigRat> &p_y);

	godot::Error SetString(const godot::String &p_s);
	godot::String String() const;
	_FORCE_INLINE_ godot::String _to_string() const { return String(); }
	godot::String RatString() const;
	godot::String FloatString(int64_t p_prec) const;
	godot::Pair<int64_t, bool> FloatPrec() const;
	_FORCE_INLINE_ int64_t _FloatPrec_bind() const { return FloatPrec().first; }
	_FORCE_INLINE_ bool _FloatPrecExact_bind() const { return FloatPrec().second; }
};
