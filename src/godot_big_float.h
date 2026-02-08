#pragma once

#include "godot_big_shared.h"

class BigInt;
class BigRat;
struct BigDecimal;

class BigFloat : public GODOT_BIG_BASE_CLASS {
	GDCLASS(BigFloat, GODOT_BIG_BASE_CLASS);

protected:
	static void _bind_methods();

public:
	typedef BigAccuracy Accuracy;
	enum RoundingMode : uint8_t {
		TO_NEAREST_EVEN = 0,
		TO_NEAREST_AWAY = 1,
		TO_ZERO = 2,
		AWAY_FROM_ZERO = 3,
		TO_NEGATIVE_INF = 4,
		TO_POSITIVE_INF = 5,
	};
	enum Form : uint8_t {
		FORM_ZERO = 0,
		FORM_FINITE = 1,
		FORM_INF = 2,
	};

	static constexpr int64_t MAX_EXP = INT32_MAX;
	static constexpr int64_t MIN_EXP = INT32_MIN;
	static constexpr uint64_t MAX_PREC = UINT32_MAX;

	enum Format : char {
		FORMAT_SCIENTIFIC = 'e',
		FORMAT_SCIENTIFIC_CAPS = 'E',
		FORMAT_NUMBER = 'f',
		FORMAT_AUTO = 'g',
		FORMAT_AUTO_CAPS = 'G',
		FORMAT_HEX = 'x',
		FORMAT_HEX_NORM = 'p',
		FORMAT_DECIMAL = 'b',
	};

private:
	uint32_t _prec = 0;
	RoundingMode _mode = TO_NEAREST_EVEN;
	Accuracy _acc = ACCURACY_EXACT;
	Form _form = FORM_ZERO;
	bool _neg = false;
	godot::PackedInt64Array _mant;
	int32_t _exp = 0;
	friend struct BigDecimal;

public:
	static godot::Ref<BigFloat> make(double p_x);

	// raw getters/setters for serialization
	inline uint32_t _get_prec() const { return _prec; }
	inline void _set_prec(uint32_t p_prec) { _prec = p_prec; }
	inline int32_t _get_exp() const { return _exp; }
	inline void _set_exp(int32_t p_exp) { _exp = p_exp; }
	inline RoundingMode _get_mode() const { return _mode; }
	inline void _set_mode(RoundingMode p_mode) { _mode = p_mode; }
	inline Accuracy _get_acc() const { return _acc; }
	inline void _set_acc(Accuracy p_acc) { _acc = p_acc; }
	inline Form _get_form() const { return _form; }
	inline void _set_form(Form p_form) { _form = p_form; }
	inline bool _is_neg() const { return _neg; }
	inline void _set_neg(bool p_neg) { _neg = p_neg; }
	inline godot::PackedInt64Array _get_mant() const { return _mant; }
	inline void _set_mant(const godot::PackedInt64Array &p_mant) { _mant = p_mant; }

	godot::Ref<BigFloat> SetPrec(uint64_t prec);
	godot::Ref<BigFloat> SetMode(RoundingMode mode);
	uint64_t Prec() const;
	uint64_t MinPrec() const;
	RoundingMode Mode() const;
	Accuracy Acc() const;
	int Sign() const;
	int64_t MantExp(const godot::Ref<BigFloat> &mant) const;
	void setExpAndRound(int64_t exp, uint64_t sbit);
	godot::Ref<BigFloat> SetMantExp(const godot::Ref<BigFloat> &mant, int64_t exp);
	bool Signbit() const;
	bool IsInf() const;
	bool IsInt() const;
#ifdef GODOT_BIG_DEBUG_FLOAT
	void validate() const;
	static void validateBinaryOperands(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
#endif
	void round(uint64_t sbit);
	godot::Ref<BigFloat> setBits64(bool neg, uint64_t x);
	godot::Ref<BigFloat> SetUint64(uint64_t x);
	godot::Ref<BigFloat> SetInt64(int64_t x);
	godot::Ref<BigFloat> SetFloat64(double x);
	godot::Ref<BigFloat> SetInt(const godot::Ref<BigInt> &x);
	godot::Ref<BigFloat> SetRat(const godot::Ref<BigRat> &x);
	godot::Ref<BigFloat> SetInf(bool signbit);
	godot::Ref<BigFloat> Set(const godot::Ref<BigFloat> &x);
	godot::Ref<BigFloat> Copy(const godot::Ref<BigFloat> &x);
	uint64_t Uint64() const;
	Accuracy Uint64Accuracy() const;
	int64_t Int64() const;
	Accuracy Int64Accuracy() const;
	float Float32() const;
	Accuracy Float32Accuracy() const;
	double Float64() const;
	Accuracy Float64Accuracy() const;
	Accuracy Int(const godot::Ref<BigInt> &z) const;
	Accuracy Rat(const godot::Ref<BigRat> &z) const;
	godot::Ref<BigFloat> Abs(const godot::Ref<BigFloat> &x);
	godot::Ref<BigFloat> Neg(const godot::Ref<BigFloat> &x);
	void uadd(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	void usub(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	void umul(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	void uquo(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	int ucmp(const godot::Ref<BigFloat> &y) const;
	godot::Ref<BigFloat> Add(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	godot::Ref<BigFloat> Sub(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	godot::Ref<BigFloat> Mul(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	godot::Ref<BigFloat> Quo(const godot::Ref<BigFloat> &x, const godot::Ref<BigFloat> &y);
	int Cmp(const godot::Ref<BigFloat> &y) const;
	int ord() const;

	godot::Ref<BigFloat> scan(const godot::PackedByteArray &buf, int64_t &i, int64_t &base);
	godot::Ref<BigFloat> pow5(uint64_t n);
	godot::Ref<BigFloat> SetString(const godot::String &s, int64_t base = 0);

	godot::String Text(Format fmt, int64_t prec) const;
	inline godot::String _to_string() const { return Text(FORMAT_AUTO, 10); }

	godot::Ref<BigFloat> Sqrt(const godot::Ref<BigFloat> &x);
	void sqrtInverse(const godot::Ref<BigFloat> &x);

	godot::String fmtE(godot::PackedByteArray &buf, char fmt, int64_t prec, BigDecimal &d) const;
	godot::String fmtF(godot::PackedByteArray &buf, int64_t prec, BigDecimal &d) const;
	godot::String fmtB(godot::PackedByteArray &buf) const;
	godot::String fmtX(godot::PackedByteArray &buf, int64_t prec) const;
	godot::String fmtP(godot::PackedByteArray &buf) const;
};
VARIANT_ENUM_CAST(BigFloat::Accuracy);
VARIANT_ENUM_CAST(BigFloat::RoundingMode);
VARIANT_ENUM_CAST(BigFloat::Form);
VARIANT_ENUM_CAST(BigFloat::Format);
