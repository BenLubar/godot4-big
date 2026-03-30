#include "godot_big_float.h"
#include "godot_big_int.h"
#include "godot_big_rat.h"

using namespace godot;

void BigFloat::_bind_methods() {
	// enumerations
	BIND_ENUM_CONSTANT(ACC_BELOW);
	BIND_ENUM_CONSTANT(ACC_EXACT);
	BIND_ENUM_CONSTANT(ACC_ABOVE);

	BIND_ENUM_CONSTANT(TO_NEAREST_EVEN);
	BIND_ENUM_CONSTANT(TO_NEAREST_AWAY);
	BIND_ENUM_CONSTANT(TO_ZERO);
	BIND_ENUM_CONSTANT(AWAY_FROM_ZERO);
	BIND_ENUM_CONSTANT(TO_NEGATIVE_INF);
	BIND_ENUM_CONSTANT(TO_POSITIVE_INF);

	BIND_ENUM_CONSTANT(FORM_ZERO);
	BIND_ENUM_CONSTANT(FORM_FINITE);
	BIND_ENUM_CONSTANT(FORM_INF);

	// serialization (Resource)
	ClassDB::bind_method(D_METHOD("_set_prec", "prec"), &BigFloat::_set_prec);
	ClassDB::bind_method(D_METHOD("_get_prec"), &BigFloat::_get_prec);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_prec"), "_set_prec", "_get_prec");
	ClassDB::bind_method(D_METHOD("_set_mode", "mode"), &BigFloat::_set_mode);
	ClassDB::bind_method(D_METHOD("_get_mode"), &BigFloat::_get_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_mode", PROPERTY_HINT_ENUM, "ToNearestEven,ToNearestAway,ToZero,AwayFromZero,ToNegativeInf,ToPositiveInf", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.RoundingMode"), "_set_mode", "_get_mode");
	ClassDB::bind_method(D_METHOD("_set_acc", "acc"), &BigFloat::_set_acc);
	ClassDB::bind_method(D_METHOD("_get_acc"), &BigFloat::_get_acc);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_acc", PROPERTY_HINT_ENUM, "Below:-1,Exact:0,Above:1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.Accuracy"), "_set_acc", "_get_acc");
	ClassDB::bind_method(D_METHOD("_set_form", "form"), &BigFloat::_set_form);
	ClassDB::bind_method(D_METHOD("_get_form"), &BigFloat::_get_form);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_form", PROPERTY_HINT_ENUM, "zero,finite,inf", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.Form"), "_set_form", "_get_form");
	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigFloat::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigFloat::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg"), "_set_neg", "_is_neg");
	ClassDB::bind_method(D_METHOD("_set_mant", "mant"), &BigFloat::_set_mant);
	ClassDB::bind_method(D_METHOD("_get_mant"), &BigFloat::_get_mant);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_mant"), "_set_mant", "_get_mant");
	ClassDB::bind_method(D_METHOD("_set_exp", "exp"), &BigFloat::_set_exp);
	ClassDB::bind_method(D_METHOD("_get_exp"), &BigFloat::_get_exp);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_exp"), "_set_exp", "_get_exp");

	// serialization (varint)
	ClassDB::bind_method(D_METHOD("to_bytes"), &BigFloat::to_bytes);
	ClassDB::bind_method(D_METHOD("from_bytes", "bytes", "offset"), &BigFloat::from_bytes, DEFVAL(0));

	// constructor
	ClassDB::bind_static_method(get_class_static(), D_METHOD("NewFloat", "x"), &BigFloat::NewFloat);

	// limits
	BIND_CONSTANT(MAX_EXP);
	BIND_CONSTANT(MIN_EXP);
	BIND_CONSTANT(MAX_PREC);

	// raw data access
	ClassDB::bind_method(D_METHOD("SetPrec", "prec"), &BigFloat::SetPrec);
	ClassDB::bind_method(D_METHOD("SetMode", "mode"), &BigFloat::SetMode);
	ClassDB::bind_method(D_METHOD("Prec"), &BigFloat::Prec);
	ClassDB::bind_method(D_METHOD("MinPrec"), &BigFloat::MinPrec);
	ClassDB::bind_method(D_METHOD("Mode"), &BigFloat::Mode);
	ClassDB::bind_method(D_METHOD("Acc"), &BigFloat::Acc);
	ClassDB::bind_method(D_METHOD("Sign"), &BigFloat::Sign);
	ClassDB::bind_method(D_METHOD("MantExp", "mant"), &BigFloat::MantExp);
	ClassDB::bind_method(D_METHOD("SetMantExp", "mant", "exp"), &BigFloat::SetMantExp);

	// classification
	ClassDB::bind_method(D_METHOD("Signbit"), &BigFloat::Signbit);
	ClassDB::bind_method(D_METHOD("IsInf"), &BigFloat::IsInf);
	ClassDB::bind_method(D_METHOD("IsInt"), &BigFloat::IsInt);

	// set value
	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigFloat::SetUint64);
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigFloat::SetInt64);
	ClassDB::bind_method(D_METHOD("SetFloat64", "x"), &BigFloat::SetFloat64);
	ClassDB::bind_method(D_METHOD("SetInt", "x"), &BigFloat::SetInt);
	ClassDB::bind_method(D_METHOD("SetRat", "x"), &BigFloat::SetRat);
	ClassDB::bind_method(D_METHOD("SetInf", "signbit"), &BigFloat::SetInf);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigFloat::Set);
	ClassDB::bind_method(D_METHOD("Copy", "x"), &BigFloat::Copy);

	// convert to other types
	ClassDB::bind_method(D_METHOD("Uint64"), &BigFloat::_Uint64_bind);
	ClassDB::bind_method(D_METHOD("Uint64Accuracy"), &BigFloat::_Uint64Accuracy_bind);
	ClassDB::bind_method(D_METHOD("Int64"), &BigFloat::_Int64_bind);
	ClassDB::bind_method(D_METHOD("Int64Accuracy"), &BigFloat::_Int64Accuracy_bind);
	ClassDB::bind_method(D_METHOD("Float32"), &BigFloat::_Float32_bind);
	ClassDB::bind_method(D_METHOD("Float32Accuracy"), &BigFloat::_Float32Accuracy_bind);
	ClassDB::bind_method(D_METHOD("Float64"), &BigFloat::_Float64_bind);
	ClassDB::bind_method(D_METHOD("Float64Accuracy"), &BigFloat::_Float64Accuracy_bind);
	ClassDB::bind_method(D_METHOD("Int", "z"), &BigFloat::Int);
	ClassDB::bind_method(D_METHOD("Rat", "z"), &BigFloat::Rat);

	// arithmetic
	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigFloat::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigFloat::Neg);
	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigFloat::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigFloat::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigFloat::Mul);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigFloat::Quo);
	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigFloat::Cmp);

	// formatting
	BIND_ENUM_CONSTANT(FORMAT_SCIENTIFIC);
	BIND_ENUM_CONSTANT(FORMAT_SCIENTIFIC_UPPER);
	BIND_ENUM_CONSTANT(FORMAT_PLAIN);
	BIND_ENUM_CONSTANT(FORMAT_AUTO);
	BIND_ENUM_CONSTANT(FORMAT_AUTO_UPPER);
	BIND_ENUM_CONSTANT(FORMAT_HEX);
	BIND_ENUM_CONSTANT(FORMAT_HEX_NORMAL);
	BIND_ENUM_CONSTANT(FORMAT_DECIMAL);

	ClassDB::bind_method(D_METHOD("SetString", "s", "base"), &BigFloat::SetString, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("String", "format", "prec"), &BigFloat::String, DEFVAL(FORMAT_AUTO), DEFVAL(10));

	// square root!
	ClassDB::bind_method(D_METHOD("Sqrt", "x"), &BigFloat::Sqrt);
}

void BigFloat::_set_prec(uint32_t p_prec) {
	if (_prec != p_prec) {
		_prec = p_prec;
		emit_changed();
	}
}
uint32_t BigFloat::_get_prec() const {
	return _prec;
}
void BigFloat::_set_mode(RoundingMode p_mode) {
	if (_mode != p_mode) {
		_mode = p_mode;
		emit_changed();
	}
}
BigFloat::RoundingMode BigFloat::_get_mode() const {
	return _mode;
}
void BigFloat::_set_acc(BigAccuracy p_acc) {
	if (_acc != p_acc) {
		_acc = p_acc;
		emit_changed();
	}
}
BigAccuracy BigFloat::_get_acc() const {
	return _acc;
}
void BigFloat::_set_form(Form p_form) {
	if (_form != p_form) {
		_form = p_form;
		emit_changed();
	}
}
BigFloat::Form BigFloat::_get_form() const {
	return _form;
}
void BigFloat::_set_neg(bool p_neg) {
	if (_neg != p_neg) {
		_neg = p_neg;
		emit_changed();
	}
}
bool BigFloat::_is_neg() const {
	return _neg;
}
void BigFloat::_set_mant(const PackedInt64Array &p_mant) {
	if (_mant.array != p_mant) {
		_mant.array = p_mant;
		emit_changed();
	}
}
PackedInt64Array BigFloat::_get_mant() const {
	return _mant.array;
}
void BigFloat::_set_exp(int32_t p_exp) {
	if (_exp != p_exp) {
		_exp = p_exp;
		emit_changed();
	}
}
int32_t BigFloat::_get_exp() const {
	return _exp;
}

void BigInt::_bind_methods() {
	// serialization (Resource)
	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigInt::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigInt::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg"), "_set_neg", "_is_neg");
	ClassDB::bind_method(D_METHOD("_set_abs", "abs"), &BigInt::_set_abs);
	ClassDB::bind_method(D_METHOD("_get_abs"), &BigInt::_get_abs);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_abs"), "_set_abs", "_get_abs");

	// serialization (varint)
	ClassDB::bind_method(D_METHOD("to_uvarint"), &BigInt::to_uvarint);
	ClassDB::bind_method(D_METHOD("from_uvarint", "bytes", "offset"), &BigInt::from_uvarint, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("to_svarint"), &BigInt::to_svarint);
	ClassDB::bind_method(D_METHOD("from_svarint", "bytes", "offset"), &BigInt::from_svarint, DEFVAL(0));

	// sign
	ClassDB::bind_method(D_METHOD("Sign"), &BigInt::Sign);

	// set value
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigInt::SetInt64);
	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigInt::SetUint64);
	ClassDB::bind_static_method(get_class_static(), D_METHOD("NewInt", "x"), &BigInt::NewInt);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigInt::Set);

	// arithmetic
	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigInt::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigInt::Neg);
	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigInt::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigInt::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigInt::Mul);
	ClassDB::bind_method(D_METHOD("MulRange", "a", "b"), &BigInt::MulRange);
	ClassDB::bind_method(D_METHOD("Binomial", "n", "k"), &BigInt::Binomial);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigInt::Quo);
	ClassDB::bind_method(D_METHOD("Rem", "x", "y"), &BigInt::Rem);
	ClassDB::bind_method(D_METHOD("QuoRem", "x", "y", "r"), &BigInt::QuoRem);
	ClassDB::bind_method(D_METHOD("Div", "x", "y"), &BigInt::Div);
	ClassDB::bind_method(D_METHOD("Mod", "x", "y"), &BigInt::Mod);
	ClassDB::bind_method(D_METHOD("DivMod", "x", "y", "m"), &BigInt::DivMod);

	// comparison
	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigInt::Cmp);
	ClassDB::bind_method(D_METHOD("CmpAbs", "y"), &BigInt::CmpAbs);

	// conversion to native numeric types
	ClassDB::bind_method(D_METHOD("Int64"), &BigInt::Int64);
	ClassDB::bind_method(D_METHOD("Uint64"), &BigInt::Uint64);
	ClassDB::bind_method(D_METHOD("IsInt64"), &BigInt::IsInt64);
	ClassDB::bind_method(D_METHOD("IsUint64"), &BigInt::IsUint64);
	ClassDB::bind_method(D_METHOD("Float64"), &BigInt::_Float64_bind);
	ClassDB::bind_method(D_METHOD("Float64Accuracy"), &BigInt::_Float64Accuracy_bind);

	// binary conversion
	ClassDB::bind_method(D_METHOD("SetBytes", "bytes"), &BigInt::SetBytes);
	ClassDB::bind_method(D_METHOD("Bytes"), &BigInt::Bytes);
	ClassDB::bind_method(D_METHOD("BitLen"), &BigInt::BitLen);
	ClassDB::bind_method(D_METHOD("TrailingZeroBits"), &BigInt::TrailingZeroBits);

	// cool math tricks
	ClassDB::bind_method(D_METHOD("Exp", "x", "y", "m"), &BigInt::Exp, DEFVAL(nullptr));
	ClassDB::bind_method(D_METHOD("GCD", "x", "y", "a", "b"), &BigInt::GCD);
	ClassDB::bind_method(D_METHOD("Rand", "rnd", "n"), &BigInt::_Rand_bind);

	// fancy math tricks
	ClassDB::bind_method(D_METHOD("ModInverse", "g", "n"), &BigInt::ModInverse);
	ClassDB::bind_static_method(get_class_static(), D_METHOD("Jacobi", "x", "y"), &BigInt::Jacobi);
	ClassDB::bind_method(D_METHOD("ModSqrt", "x", "p"), &BigInt::ModSqrt);

	// bitwise
	ClassDB::bind_method(D_METHOD("Lsh", "x", "n"), &BigInt::Lsh);
	ClassDB::bind_method(D_METHOD("Rsh", "x", "n"), &BigInt::Rsh);
	ClassDB::bind_method(D_METHOD("Bit", "i"), &BigInt::Bit);
	ClassDB::bind_method(D_METHOD("SetBit", "x", "i", "b"), &BigInt::SetBit);
	ClassDB::bind_method(D_METHOD("And", "x", "y"), &BigInt::And);
	ClassDB::bind_method(D_METHOD("AndNot", "x", "y"), &BigInt::AndNot);
	ClassDB::bind_method(D_METHOD("Or", "x", "y"), &BigInt::Or);
	ClassDB::bind_method(D_METHOD("Xor", "x", "y"), &BigInt::Xor);
	ClassDB::bind_method(D_METHOD("Not", "x"), &BigInt::Not);

	// square root!
	ClassDB::bind_method(D_METHOD("Sqrt", "x"), &BigInt::Sqrt);

	// string conversion
	BIND_CONSTANT(MAX_BASE);
	ClassDB::bind_method(D_METHOD("SetString", "s", "base"), &BigInt::SetString, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("String", "base"), &BigInt::String, DEFVAL(10));

	// prime test
	ClassDB::bind_method(D_METHOD("ProbablyPrime", "n"), &BigInt::ProbablyPrime);
}

void BigInt::_set_neg(bool p_neg) {
	if (_neg != p_neg) {
		_neg = p_neg;
		emit_changed();
	}
}
bool BigInt::_is_neg() const {
	return _neg;
}
void BigInt::_set_abs(const PackedInt64Array &p_abs) {
	if (_abs.array != p_abs) {
		_abs.array = p_abs;
		emit_changed();
	}
}
PackedInt64Array BigInt::_get_abs() const {
	return _abs.array;
}

void BigRat::_bind_methods() {
	// serialization (Resource)
	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigRat::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigRat::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg"), "_set_neg", "_is_neg");
	ClassDB::bind_method(D_METHOD("_set_a", "a"), &BigRat::_set_a);
	ClassDB::bind_method(D_METHOD("_get_a"), &BigRat::_get_a);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_a"), "_set_a", "_get_a");
	ClassDB::bind_method(D_METHOD("_set_b", "b"), &BigRat::_set_b);
	ClassDB::bind_method(D_METHOD("_get_b"), &BigRat::_get_b);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_b"), "_set_b", "_get_b");

	// serialization (varint)
	ClassDB::bind_method(D_METHOD("to_bytes"), &BigRat::to_bytes);
	ClassDB::bind_method(D_METHOD("from_bytes", "bytes", "offset"), &BigRat::from_bytes, DEFVAL(0));

	// constructor
	ClassDB::bind_static_method(get_class_static(), D_METHOD("NewRat", "a", "b"), &BigRat::NewRat);

	// set value
	ClassDB::bind_method(D_METHOD("SetFloat64", "f"), &BigRat::SetFloat64);
	ClassDB::bind_method(D_METHOD("SetFrac", "a", "b"), &BigRat::SetFrac);
	ClassDB::bind_method(D_METHOD("SetFrac64", "a", "b"), &BigRat::SetFrac64);
	ClassDB::bind_method(D_METHOD("SetInt", "x"), &BigRat::SetInt);
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigRat::SetInt64);
	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigRat::SetUint64);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigRat::Set);

	// convert to floats
	ClassDB::bind_method(D_METHOD("Float32"), &BigRat::_Float32_bind);
	ClassDB::bind_method(D_METHOD("Float32Exact"), &BigRat::_Float32Exact_bind);
	ClassDB::bind_method(D_METHOD("Float64"), &BigRat::_Float64_bind);
	ClassDB::bind_method(D_METHOD("Float64Exact"), &BigRat::_Float64Exact_bind);

	// arithmetic
	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigRat::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigRat::Neg);
	ClassDB::bind_method(D_METHOD("Inv", "x"), &BigRat::Inv);
	ClassDB::bind_method(D_METHOD("Sign"), &BigRat::Sign);
	ClassDB::bind_method(D_METHOD("IsInt"), &BigRat::IsInt);

	ClassDB::bind_method(D_METHOD("Num", "a"), &BigRat::Num);
	ClassDB::bind_method(D_METHOD("Denom", "b"), &BigRat::Denom);
	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigRat::Cmp);

	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigRat::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigRat::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigRat::Mul);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigRat::Quo);

	// string conversion
	ClassDB::bind_method(D_METHOD("SetString", "s"), &BigRat::SetString);
	ClassDB::bind_method(D_METHOD("String"), &BigRat::String);
	ClassDB::bind_method(D_METHOD("RatString"), &BigRat::RatString);
	ClassDB::bind_method(D_METHOD("FloatString", "prec"), &BigRat::FloatString);
	ClassDB::bind_method(D_METHOD("FloatPrec"), &BigRat::_FloatPrec_bind);
	ClassDB::bind_method(D_METHOD("FloatPrecExact"), &BigRat::_FloatPrecExact_bind);
}

void BigRat::_set_neg(bool p_neg) {
	if (_neg != p_neg) {
		_neg = p_neg;
		emit_changed();
	}
}
bool BigRat::_is_neg() const {
	return _neg;
}
void BigRat::_set_a(const PackedInt64Array &p_a) {
	if (_a.array != p_a) {
		_a.array = p_a;
		emit_changed();
	}
}
PackedInt64Array BigRat::_get_a() const {
	return _a.array;
}
void BigRat::_set_b(const PackedInt64Array &p_b) {
	if (_b.array != p_b) {
		_b.array = p_b;
		emit_changed();
	}
}
PackedInt64Array BigRat::_get_b() const {
	return _b.array;
}
