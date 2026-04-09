#pragma once

#include "godot_big_naturals.h"

#include <godot_cpp/classes/resource.hpp>

class BigInt;
class BigRat;
struct BigDecimal;

class BigFloat : public godot::Resource {
	GDCLASS(BigFloat, godot::Resource);

protected:
	static void _bind_methods();

public:
	using Accuracy = BigAccuracy;

	enum RoundingMode : uint8_t {
		TO_NEAREST_EVEN = 0, // == IEEE 754-2008 roundTiesToEven
		TO_NEAREST_AWAY = 1, // == IEEE 754-2008 roundTiesToAway
		TO_ZERO = 2, // == IEEE 754-2008 roundTowardZero
		AWAY_FROM_ZERO = 3, // no IEEE 754-2008 equivalent
		TO_NEGATIVE_INF = 4, // == IEEE 754-2008 roundTowardNegative
		TO_POSITIVE_INF = 5, // == IEEE 754-2008 roundTowardPositive
	};

	enum Form : uint8_t {
		FORM_ZERO = 0,
		FORM_FINITE = 1,
		FORM_INF = 2,
	};

	uint32_t _prec = 0;
	RoundingMode _mode = TO_NEAREST_EVEN;
	BigAccuracy _acc = ACC_EXACT;
	Form _form = FORM_ZERO;
	bool _neg = false;
	BigNat _mant;
	int32_t _exp = 0;

	void _set_prec(uint32_t p_prec);
	[[nodiscard]] uint32_t _get_prec() const;
	void _set_mode(RoundingMode p_mode);
	[[nodiscard]] RoundingMode _get_mode() const;
	void _set_acc(BigAccuracy p_acc);
	[[nodiscard]] BigAccuracy _get_acc() const;
	void _set_form(Form p_form);
	[[nodiscard]] Form _get_form() const;
	void _set_neg(bool p_neg);
	[[nodiscard]] bool _is_neg() const;
	void _set_mant(const godot::PackedInt64Array &p_mant);
	[[nodiscard]] godot::PackedInt64Array _get_mant() const;
	void _set_exp(int32_t p_exp);
	[[nodiscard]] int32_t _get_exp() const;

	[[nodiscard]] godot::PackedByteArray to_bytes() const;
	int64_t from_bytes(const godot::PackedByteArray &p_bytes, int64_t p_offset = 0);

	static godot::Ref<BigFloat> NewFloat(double p_x);

	static constexpr int32_t MAX_EXP = INT32_MAX;
	static constexpr int32_t MIN_EXP = INT32_MIN;
	static constexpr uint32_t MAX_PREC = UINT32_MAX;

	void SetPrec(uint64_t p_prec);
	void SetMode(RoundingMode p_mode);
	[[nodiscard]] uint32_t Prec() const;
	[[nodiscard]] uint32_t MinPrec() const;
	[[nodiscard]] RoundingMode Mode() const;
	[[nodiscard]] BigAccuracy Acc() const;
	[[nodiscard]] int Sign() const;
	[[nodiscard]] int32_t MantExp(const godot::Ref<BigFloat> &r_mant) const;
	void SetMantExp(const godot::Ref<BigFloat> &p_mant, int64_t p_exp);
	void _setExpAndRound(int64_t p_exp, uint64_t p_sbit);

	[[nodiscard]] bool Signbit() const;
	[[nodiscard]] bool IsInf() const;
	[[nodiscard]] bool IsInt() const;
	void _validate() const;
	void _round(BigWord p_sbit);

	void _setBits64(bool p_neg, uint64_t p_x);
	void SetUint64(uint64_t p_x);
	void SetInt64(int64_t p_x);
	godot::Error SetFloat64(double p_x);
	void SetInt(const godot::Ref<BigInt> &p_x);
	void SetRat(const godot::Ref<BigRat> &p_x);
	void SetInf(bool p_signbit);
	void Set(const godot::Ref<BigFloat> &p_x);
	void Copy(const godot::Ref<BigFloat> &p_x);

	[[nodiscard]] godot::Pair<uint64_t, BigAccuracy> Uint64() const;
	[[nodiscard]] _FORCE_INLINE_ uint64_t Uint64_bind() const { return Uint64().first; }
	[[nodiscard]] _FORCE_INLINE_ BigAccuracy Uint64Accuracy_bind() const { return Uint64().second; }
	[[nodiscard]] godot::Pair<int64_t, BigAccuracy> Int64() const;
	[[nodiscard]] _FORCE_INLINE_ int64_t Int64_bind() const { return Int64().first; }
	[[nodiscard]] _FORCE_INLINE_ BigAccuracy Int64Accuracy_bind() const { return Int64().second; }
	[[nodiscard]] godot::Pair<float, BigAccuracy> Float32() const;
	[[nodiscard]] _FORCE_INLINE_ float Float32_bind() const { return Float32().first; }
	[[nodiscard]] _FORCE_INLINE_ BigAccuracy Float32Accuracy_bind() const { return Float32().second; }
	[[nodiscard]] godot::Pair<double, BigAccuracy> Float64() const;
	[[nodiscard]] _FORCE_INLINE_ double Float64_bind() const { return Float64().first; }
	[[nodiscard]] _FORCE_INLINE_ BigAccuracy Float64Accuracy_bind() const { return Float64().second; }
	[[nodiscard]] BigAccuracy Int(const godot::Ref<BigInt> &r_z) const;
	[[nodiscard]] BigAccuracy Rat(const godot::Ref<BigRat> &r_z) const;

	void Abs(const godot::Ref<BigFloat> &p_x);
	void Neg(const godot::Ref<BigFloat> &p_x);
	void _uadd(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	void _usub(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	void _umul(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	void _uquo(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	[[nodiscard]] int _ucmp(const godot::Ref<BigFloat> &p_y) const;
	[[nodiscard]] int _ord() const;
	godot::Error Add(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	godot::Error Sub(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	godot::Error Mul(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	godot::Error Quo(const godot::Ref<BigFloat> &p_x, const godot::Ref<BigFloat> &p_y);
	[[nodiscard]] int Cmp(const godot::Ref<BigFloat> &p_y) const;

	enum Format {
		FORMAT_SCIENTIFIC = 'e',
		FORMAT_SCIENTIFIC_UPPER = 'E',
		FORMAT_PLAIN = 'f',
		FORMAT_AUTO = 'g',
		FORMAT_AUTO_UPPER = 'G',
		FORMAT_HEX = 'x',
		FORMAT_HEX_NORMAL = 'p',
		FORMAT_DECIMAL = 'b',
	};

	godot::Error _scan(const godot::String &s, int64_t &off, int64_t &base);
	void _pow5(uint64_t p_n);
	godot::Error SetString(const godot::String &p_s, int64_t p_base = 0);
	[[nodiscard]] godot::String _fmtB(godot::PackedByteArray &buf) const;
	[[nodiscard]] godot::String _fmtP(godot::PackedByteArray &buf) const;
	[[nodiscard]] godot::String _fmtX(godot::PackedByteArray &buf, int64_t prec) const;
	[[nodiscard]] static godot::String _fmtE(godot::PackedByteArray &buf, Format fmt, int64_t prec, BigDecimal &d);
	[[nodiscard]] static godot::String _fmtF(godot::PackedByteArray &buf, int64_t prec, BigDecimal &d);
	[[nodiscard]] godot::String String(Format p_format = FORMAT_AUTO, int64_t p_prec = 10) const;
	[[nodiscard]] _FORCE_INLINE_ godot::String _to_string() const { return String(); }

	void _sqrtInverse(const godot::Ref<BigFloat> &p_x);
	godot::Error Sqrt(const godot::Ref<BigFloat> &p_x);
};
VARIANT_ENUM_CAST(BigFloat::Accuracy);
VARIANT_ENUM_CAST(BigFloat::RoundingMode);
VARIANT_ENUM_CAST(BigFloat::Form);
VARIANT_ENUM_CAST(BigFloat::Format);
