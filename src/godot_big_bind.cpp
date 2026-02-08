#include <godot_cpp/core/class_db.hpp>

#include "godot_big_int.h"
#include "godot_big_rat.h"
#include "godot_big_float.h"

using namespace godot;

void BigInt::_bind_methods() {
	BIND_CONSTANT(MAX_BASE);

	ClassDB::bind_static_method(get_class_static(), D_METHOD("make", "x"), &BigInt::make);

	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigInt::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigInt::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_neg", "_is_neg");

	ClassDB::bind_method(D_METHOD("_set_abs", "abs"), &BigInt::_set_abs);
	ClassDB::bind_method(D_METHOD("_get_abs"), &BigInt::_get_abs);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_abs", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_abs", "_get_abs");

	// Assignment
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigInt::SetInt64);
	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigInt::SetUint64);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigInt::Set);

	// Arithmetic
	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigInt::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigInt::Neg);
	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigInt::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigInt::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigInt::Mul);

	// Division
	ClassDB::bind_method(D_METHOD("Sign"), &BigInt::Sign);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigInt::Quo);
	ClassDB::bind_method(D_METHOD("Rem", "x", "y"), &BigInt::Rem);
	ClassDB::bind_method(D_METHOD("QuoRem", "x", "y"), &BigInt::QuoRem);
	ClassDB::bind_method(D_METHOD("Div", "x", "y"), &BigInt::Div);
	ClassDB::bind_method(D_METHOD("Mod", "x", "y"), &BigInt::Mod);
	ClassDB::bind_method(D_METHOD("DivMod", "x", "y"), &BigInt::DivMod);

	// Comparison
	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigInt::Cmp);
	ClassDB::bind_method(D_METHOD("CmpAbs", "y"), &BigInt::CmpAbs);

	// Conversion
	ClassDB::bind_method(D_METHOD("Int64"), &BigInt::Int64);
	ClassDB::bind_method(D_METHOD("Uint64"), &BigInt::Uint64);
	ClassDB::bind_method(D_METHOD("IsInt64"), &BigInt::IsInt64);
	ClassDB::bind_method(D_METHOD("IsUint64"), &BigInt::IsUint64);
	ClassDB::bind_method(D_METHOD("Float64"), &BigInt::Float64);
	ClassDB::bind_method(D_METHOD("Float64Accuracy"), &BigInt::Float64Accuracy);
	ClassDB::bind_method(D_METHOD("Text", "base"), &BigInt::Text);

	// Serialization
	ClassDB::bind_method(D_METHOD("SetString", "s", "base"), &BigInt::SetString);
	ClassDB::bind_method(D_METHOD("SetBytes", "buf"), &BigInt::SetBytes);
	ClassDB::bind_method(D_METHOD("Bytes"), &BigInt::Bytes);
	ClassDB::bind_method(D_METHOD("BitLen"), &BigInt::BitLen);
	ClassDB::bind_method(D_METHOD("TrailingZeroBits"), &BigInt::TrailingZeroBits);

	// Utility
	ClassDB::bind_method(D_METHOD("MulRange", "a", "b"), &BigInt::MulRange);
	ClassDB::bind_method(D_METHOD("Binomial", "n", "k"), &BigInt::Binomial);
	ClassDB::bind_method(D_METHOD("Exp", "x", "y", "m"), &BigInt::Exp);
	ClassDB::bind_method(D_METHOD("GCD", "x", "y", "a", "b"), &BigInt::GCD);
	ClassDB::bind_method(D_METHOD("Rand", "rnd", "n"), &BigInt::Rand);
	ClassDB::bind_method(D_METHOD("ModInverse", "g", "n"), &BigInt::ModInverse);
	ClassDB::bind_static_method(get_class_static(), D_METHOD("Jacobi", "x", "y"), &BigInt::Jacobi);
	ClassDB::bind_method(D_METHOD("ModSqrt", "x", "p"), &BigInt::ModSqrt);
	ClassDB::bind_method(D_METHOD("Sqrt", "x"), &BigInt::Sqrt);
	ClassDB::bind_method(D_METHOD("ProbablyPrime", "n"), &BigInt::ProbablyPrime);

	// Bitwise
	ClassDB::bind_method(D_METHOD("Lsh", "x", "n"), &BigInt::Lsh);
	ClassDB::bind_method(D_METHOD("Rsh", "x", "n"), &BigInt::Rsh);
	ClassDB::bind_method(D_METHOD("Bit", "i"), &BigInt::Bit);
	ClassDB::bind_method(D_METHOD("SetBit", "x", "i", "b"), &BigInt::SetBit);
	ClassDB::bind_method(D_METHOD("And", "x", "y"), &BigInt::And);
	ClassDB::bind_method(D_METHOD("AndNot", "x", "y"), &BigInt::AndNot);
	ClassDB::bind_method(D_METHOD("Or", "x", "y"), &BigInt::Or);
	ClassDB::bind_method(D_METHOD("Xor", "x", "y"), &BigInt::Xor);
	ClassDB::bind_method(D_METHOD("Not", "x"), &BigInt::Not);
}

void BigRat::_bind_methods() {
	ClassDB::bind_static_method(get_class_static(), D_METHOD("make", "a", "b"), &BigRat::make);

	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigRat::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigRat::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_neg", "_is_neg");

	ClassDB::bind_method(D_METHOD("_set_a", "a"), &BigRat::_set_a);
	ClassDB::bind_method(D_METHOD("_get_a"), &BigRat::_get_a);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_a", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_a", "_get_a");

	ClassDB::bind_method(D_METHOD("_set_b", "b"), &BigRat::_set_b);
	ClassDB::bind_method(D_METHOD("_get_b"), &BigRat::_get_b);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_b", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_b", "_get_b");

	ClassDB::bind_method(D_METHOD("SetFloat64", "f"), &BigRat::SetFloat64);
	ClassDB::bind_method(D_METHOD("Float32"), &BigRat::Float32);
	ClassDB::bind_method(D_METHOD("Float32Exact"), &BigRat::Float32Exact);
	ClassDB::bind_method(D_METHOD("Float64"), &BigRat::Float64);
	ClassDB::bind_method(D_METHOD("Float64Exact"), &BigRat::Float64Exact);
	ClassDB::bind_method(D_METHOD("SetFrac", "a", "b"), &BigRat::SetFrac);
	ClassDB::bind_method(D_METHOD("SetFrac64", "a", "b"), &BigRat::SetFrac64);
	ClassDB::bind_method(D_METHOD("SetInt", "x"), &BigRat::SetInt);
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigRat::SetInt64);
	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigRat::SetUint64);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigRat::Set);

	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigRat::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigRat::Neg);
	ClassDB::bind_method(D_METHOD("Inv", "x"), &BigRat::Inv);

	ClassDB::bind_method(D_METHOD("Sign"), &BigRat::Sign);
	ClassDB::bind_method(D_METHOD("IsInt"), &BigRat::IsInt);
	ClassDB::bind_method(D_METHOD("Num"), &BigRat::Num);
	ClassDB::bind_method(D_METHOD("Denom"), &BigRat::Denom);

	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigRat::Cmp);
	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigRat::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigRat::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigRat::Mul);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigRat::Quo);

	ClassDB::bind_method(D_METHOD("SetString", "s"), &BigRat::SetString);
	ClassDB::bind_method(D_METHOD("String"), &BigRat::String);
	ClassDB::bind_method(D_METHOD("RatString"), &BigRat::RatString);
	ClassDB::bind_method(D_METHOD("FloatString", "prec"), &BigRat::FloatString);
}

void BigFloat::_bind_methods() {
	BIND_CONSTANT(MAX_EXP);
	BIND_CONSTANT(MIN_EXP);
	BIND_CONSTANT(MAX_PREC);

	BIND_ENUM_CONSTANT(ACCURACY_BELOW);
	BIND_ENUM_CONSTANT(ACCURACY_EXACT);
	BIND_ENUM_CONSTANT(ACCURACY_ABOVE);

	BIND_ENUM_CONSTANT(TO_NEAREST_EVEN);
	BIND_ENUM_CONSTANT(TO_NEAREST_AWAY);
	BIND_ENUM_CONSTANT(TO_ZERO);
	BIND_ENUM_CONSTANT(AWAY_FROM_ZERO);
	BIND_ENUM_CONSTANT(TO_NEGATIVE_INF);
	BIND_ENUM_CONSTANT(TO_POSITIVE_INF);

	BIND_ENUM_CONSTANT(FORM_ZERO);
	BIND_ENUM_CONSTANT(FORM_FINITE);
	BIND_ENUM_CONSTANT(FORM_INF);

	BIND_ENUM_CONSTANT(FORMAT_SCIENTIFIC);
	BIND_ENUM_CONSTANT(FORMAT_SCIENTIFIC_CAPS);
	BIND_ENUM_CONSTANT(FORMAT_NUMBER);
	BIND_ENUM_CONSTANT(FORMAT_AUTO);
	BIND_ENUM_CONSTANT(FORMAT_AUTO_CAPS);
	BIND_ENUM_CONSTANT(FORMAT_HEX);
	BIND_ENUM_CONSTANT(FORMAT_HEX_NORM);
	BIND_ENUM_CONSTANT(FORMAT_DECIMAL);

	ClassDB::bind_static_method(get_class_static(), D_METHOD("make", "x"), &BigFloat::make);

	ClassDB::bind_method(D_METHOD("_set_prec", "prec"), &BigFloat::_set_prec);
	ClassDB::bind_method(D_METHOD("_get_prec"), &BigFloat::_get_prec);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_prec", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_prec", "_get_prec");

	ClassDB::bind_method(D_METHOD("_set_exp", "exp"), &BigFloat::_set_exp);
	ClassDB::bind_method(D_METHOD("_get_exp"), &BigFloat::_get_exp);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_exp", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_exp", "_get_exp");

	ClassDB::bind_method(D_METHOD("_set_mode", "mode"), &BigFloat::_set_mode);
	ClassDB::bind_method(D_METHOD("_get_mode"), &BigFloat::_get_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_mode", PROPERTY_HINT_ENUM, "To Nearest Even,To Nearest Away,To Zero,Away From Zero,To Negative Inf,To Positive Inf", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.RoundingMode"), "_set_mode", "_get_mode");

	ClassDB::bind_method(D_METHOD("_set_acc", "acc"), &BigFloat::_set_acc);
	ClassDB::bind_method(D_METHOD("_get_acc"), &BigFloat::_get_acc);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_acc", PROPERTY_HINT_ENUM, "Below:-1,Exact:0,Above:1", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.Accuracy"), "_set_acc", "_get_acc");

	ClassDB::bind_method(D_METHOD("_set_form", "form"), &BigFloat::_set_form);
	ClassDB::bind_method(D_METHOD("_get_form"), &BigFloat::_get_form);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_form", PROPERTY_HINT_ENUM, "Zero,Finite,Inf", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.Form"), "_set_form", "_get_form");

	ClassDB::bind_method(D_METHOD("_set_neg", "neg"), &BigFloat::_set_neg);
	ClassDB::bind_method(D_METHOD("_is_neg"), &BigFloat::_is_neg);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_neg", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_neg", "_is_neg");

	ClassDB::bind_method(D_METHOD("_set_mant", "mant"), &BigFloat::_set_mant);
	ClassDB::bind_method(D_METHOD("_get_mant"), &BigFloat::_get_mant);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "_mant", PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_STORAGE), "_set_mant", "_get_mant");

	ClassDB::bind_method(D_METHOD("SetPrec", "prec"), &BigFloat::SetPrec);
	ClassDB::bind_method(D_METHOD("SetMode", "mode"), &BigFloat::SetMode);
	ClassDB::bind_method(D_METHOD("Prec"), &BigFloat::Prec);
	ClassDB::bind_method(D_METHOD("MinPrec"), &BigFloat::MinPrec);
	ClassDB::bind_method(D_METHOD("Mode"), &BigFloat::Mode);
	ClassDB::bind_method(D_METHOD("Acc"), &BigFloat::Acc);
	ClassDB::bind_method(D_METHOD("Sign"), &BigFloat::Sign);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "precision", PROPERTY_HINT_RANGE, "0,100,or_greater", PROPERTY_USAGE_EDITOR), "SetPrec", "Prec");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rounding_mode", PROPERTY_HINT_ENUM, "To Nearest Even,To Nearest Away,To Zero,Away From Zero,To Negative Inf,To Positive Inf", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_CLASS_IS_ENUM, "BigFloat.RoundingMode"), "SetMode", "Mode");

	ClassDB::bind_method(D_METHOD("MantExp", "mant"), &BigFloat::MantExp);
	ClassDB::bind_method(D_METHOD("SetMantExp", "mant", "exp"), &BigFloat::SetMantExp);
	ClassDB::bind_method(D_METHOD("Signbit"), &BigFloat::Signbit);
	ClassDB::bind_method(D_METHOD("IsInf"), &BigFloat::IsInf);
	ClassDB::bind_method(D_METHOD("IsInt"), &BigFloat::IsInt);

	ClassDB::bind_method(D_METHOD("SetUint64", "x"), &BigFloat::SetUint64);
	ClassDB::bind_method(D_METHOD("SetInt64", "x"), &BigFloat::SetInt64);
	ClassDB::bind_method(D_METHOD("SetFloat64", "x"), &BigFloat::SetFloat64);
	ClassDB::bind_method(D_METHOD("SetInt", "x"), &BigFloat::SetInt);
	ClassDB::bind_method(D_METHOD("SetRat", "x"), &BigFloat::SetRat);
	ClassDB::bind_method(D_METHOD("SetInf", "signbit"), &BigFloat::SetInf);
	ClassDB::bind_method(D_METHOD("Set", "x"), &BigFloat::Set);
	ClassDB::bind_method(D_METHOD("Copy", "x"), &BigFloat::Copy);
	ClassDB::bind_method(D_METHOD("Uint64"), &BigFloat::Uint64);
	ClassDB::bind_method(D_METHOD("Uint64Accuracy"), &BigFloat::Uint64Accuracy);
	ClassDB::bind_method(D_METHOD("Int64"), &BigFloat::Int64);
	ClassDB::bind_method(D_METHOD("Int64Accuracy"), &BigFloat::Int64Accuracy);
	ClassDB::bind_method(D_METHOD("Float64"), &BigFloat::Float64);
	ClassDB::bind_method(D_METHOD("Float64Accuracy"), &BigFloat::Float64Accuracy);
	ClassDB::bind_method(D_METHOD("Int", "z"), &BigFloat::Int);
	ClassDB::bind_method(D_METHOD("Rat", "z"), &BigFloat::Rat);

	ClassDB::bind_method(D_METHOD("Abs", "x"), &BigFloat::Abs);
	ClassDB::bind_method(D_METHOD("Neg", "x"), &BigFloat::Neg);
	ClassDB::bind_method(D_METHOD("Add", "x", "y"), &BigFloat::Add);
	ClassDB::bind_method(D_METHOD("Sub", "x", "y"), &BigFloat::Sub);
	ClassDB::bind_method(D_METHOD("Mul", "x", "y"), &BigFloat::Mul);
	ClassDB::bind_method(D_METHOD("Quo", "x", "y"), &BigFloat::Quo);

	ClassDB::bind_method(D_METHOD("Cmp", "y"), &BigFloat::Cmp);
	ClassDB::bind_method(D_METHOD("SetString", "s", "base"), &BigFloat::SetString, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("Text", "fmt", "prec"), &BigFloat::Text);

	ClassDB::bind_method(D_METHOD("Sqrt", "x"), &BigFloat::Sqrt);
}
