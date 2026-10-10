#pragma once

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/span.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <functional>

// A Word represents a single digit of a multi-precision unsigned integer.
typedef uint64_t BigWord;

// porting guide:
// _S = 8 // word size in bytes
// _W = 64 // word size in bits
// _B = UINT64_MAX + 1 // digit base
// _M = UINT64_MAX // digit mask

// Accuracy describes the rounding error produced by the most recent
// operation that generated a [Float] value, relative to the exact value.
enum BigAccuracy : int8_t {
	ACC_BELOW = -1,
	ACC_EXACT = 0,
	ACC_ABOVE = 1,
};

// RoundingMode determines how a [Float] value is rounded to the
// desired precision. Rounding may change the [Float] value; the
// rounding error is described by the [Float]'s [Accuracy].
enum BigRoundingMode : uint8_t {
	TO_NEAREST_EVEN = 0, // == IEEE 754-2008 roundTiesToEven
	TO_NEAREST_AWAY = 1, // == IEEE 754-2008 roundTiesToAway
	TO_ZERO = 2, // == IEEE 754-2008 roundTowardZero
	AWAY_FROM_ZERO = 3, // no IEEE 754-2008 equivalent
	TO_NEGATIVE_INF = 4, // == IEEE 754-2008 roundTowardNegative
	TO_POSITIVE_INF = 5, // == IEEE 754-2008 roundTowardPositive
};

struct BigDivisor;

struct BigNat {
	godot::PackedInt64Array array;

	BigNat() noexcept = default;
	explicit BigNat(const godot::PackedInt64Array &p_array) noexcept : array(p_array) {}
	explicit BigNat(godot::PackedInt64Array &&p_array) noexcept : array(std::move(p_array)) {}
	BigNat(const BigNat &p_from) noexcept = default;
	BigNat(BigNat &&p_other) noexcept = default;
	BigNat(std::initializer_list<BigWord> p_init) noexcept : array{ reinterpret_cast<std::initializer_list<int64_t> &>(p_init) } {}
	~BigNat() noexcept = default;

	_FORCE_INLINE_ BigNat &operator=(const BigNat &p_other) noexcept {
		// work around godot-cpp bug https://github.com/godotengine/godot-cpp/issues/2071
		if (this != &p_other) {
			array = p_other.array;
		}
		return *this;
	}
	_FORCE_INLINE_ BigNat &operator=(BigNat &&p_other) noexcept = default;
	[[nodiscard]] _ALWAYS_INLINE_ const BigWord &operator[](int64_t p_index) const {
		return reinterpret_cast<const BigWord &>(array[p_index]);
	}
	[[nodiscard]] _ALWAYS_INLINE_ BigWord &operator[](int64_t p_index) {
		return reinterpret_cast<BigWord &>(array[p_index]);
	}
	[[nodiscard]] _ALWAYS_INLINE_ BigWord *ptrw() {
		return reinterpret_cast<BigWord *>(array.ptrw());
	}
	[[nodiscard]] _ALWAYS_INLINE_ const BigWord *ptr() const {
		return reinterpret_cast<const BigWord *>(array.ptr());
	}
	[[nodiscard]] _FORCE_INLINE_ bool is_zero() const {
		return array.is_empty();
	}

	using CSlice = godot::Span<BigWord>;
	struct Slice {
		// We can't use std::span because it's C++20,
		// and we can't use godot::Span because it's read-only.
		// So let's reimplement Go slices, I guess.
		BigWord *array;
		int64_t length;
		int64_t capacity;

		_FORCE_INLINE_ Slice(BigWord *p_array, int64_t p_length, int64_t p_capacity) noexcept :
				array{ p_array },
				length{ p_length },
				capacity{ p_capacity } {
			DEV_ASSERT(p_array != nullptr || p_capacity == 0);
			DEV_ASSERT(p_length >= 0);
			DEV_ASSERT(p_length <= p_capacity);
		}

		[[nodiscard]] _ALWAYS_INLINE_ BigWord &operator[](int64_t p_index) {
			CRASH_BAD_INDEX(p_index, length);
			return array[p_index];
		}
		[[nodiscard]] _ALWAYS_INLINE_ uint64_t size() const {
			return static_cast<uint64_t>(length);
		}

		[[nodiscard]] _FORCE_INLINE_ Slice slice(int64_t p_start) {
			CRASH_BAD_INDEX(p_start, length + 1);
			return { array + p_start, length - p_start, capacity - p_start };
		}
		template <bool allow_enlarge = false>
		[[nodiscard]] _FORCE_INLINE_ Slice slice(int64_t p_start, int64_t p_end) {
			CRASH_BAD_INDEX(p_start, p_end + 1);
			if constexpr (allow_enlarge) {
				CRASH_BAD_INDEX(p_end, capacity + 1);
			} else {
				CRASH_BAD_INDEX(p_end, length + 1);
			}
			return { array + p_start, p_end - p_start, capacity - p_start };
		}
		[[nodiscard]] _FORCE_INLINE_ CSlice cslice() const {
			return { array, static_cast<uint64_t>(length) };
		}
		[[nodiscard]] _FORCE_INLINE_ CSlice cslice(int64_t p_start) {
			CRASH_BAD_INDEX(p_start, length + 1);
			return { array + p_start, static_cast<uint64_t>(length - p_start) };
		}
		template <bool allow_enlarge = false>
		[[nodiscard]] _FORCE_INLINE_ CSlice cslice(int64_t p_start, int64_t p_end) {
			CRASH_BAD_INDEX(p_start, p_end + 1);
			if constexpr (allow_enlarge) {
				CRASH_BAD_INDEX(p_end, capacity + 1);
			} else {
				CRASH_BAD_INDEX(p_end, length + 1);
			}
			return { array + p_start, static_cast<uint64_t>(p_end - p_start) };
		}

		_FORCE_INLINE_ void fill(BigWord p_x) {
			for (int64_t i = 0; i < length; i++) {
				array[i] = p_x;
			}
		}
		_FORCE_INLINE_ void norm() {
			while (length > 0 && array[length - 1] == 0) {
				length--;
			}
		}
	};
	[[nodiscard]] _FORCE_INLINE_ Slice slice() {
		return { ptrw(), array.size(), array.size() };
	}
	[[nodiscard]] _FORCE_INLINE_ Slice slice(int64_t p_start) {
		CRASH_BAD_INDEX(p_start, array.size() + 1);
		return { ptrw() + p_start, array.size() - p_start, array.size() - p_start };
	}
	[[nodiscard]] _FORCE_INLINE_ Slice slice(int64_t p_start, int64_t p_end) {
		CRASH_BAD_INDEX(p_start, p_end + 1);
		CRASH_BAD_INDEX(p_end, array.size() + 1);
		return { ptrw() + p_start, p_end - p_start, array.size() - p_start };
	}
	[[nodiscard]] _FORCE_INLINE_ CSlice cslice() const {
		return { ptr(), static_cast<uint64_t>(array.size()) };
	}
	[[nodiscard]] _FORCE_INLINE_ CSlice cslice(int64_t p_start) const {
		CRASH_BAD_INDEX(p_start, array.size() + 1);
		return { ptr() + p_start, static_cast<uint64_t>(array.size() - p_start) };
	}
	[[nodiscard]] _FORCE_INLINE_ CSlice cslice(int64_t p_start, int64_t p_end) const {
		CRASH_BAD_INDEX(p_start, p_end + 1);
		CRASH_BAD_INDEX(p_end, array.size() + 1);
		return { ptr() + p_start, static_cast<uint64_t>(p_end - p_start) };
	}
	explicit BigNat(const Slice &p_slice, bool p_norm = false) noexcept {
		array.resize(p_slice.length);
		memcpy(ptrw(), p_slice.array, p_slice.length * sizeof(BigWord));
		if (p_norm) {
			norm();
		}
	}
	explicit BigNat(const CSlice &p_slice, bool p_norm = false) noexcept {
		array.resize(p_slice.size());
		memcpy(ptrw(), p_slice.ptr(), p_slice.size() * sizeof(BigWord));
		if (p_norm) {
			norm();
		}
	}

	// bits.go
#ifdef _MSC_VER
#define BITMATH_CONSTEXPR
#else
#define BITMATH_CONSTEXPR constexpr
#endif
	static BITMATH_CONSTEXPR void bits_Add(BigWord p_x, BigWord p_y, BigWord p_carry, BigWord &r_out, BigWord &r_carryout) noexcept;
	static BITMATH_CONSTEXPR void bits_Sub(BigWord p_x, BigWord p_y, BigWord p_borrow, BigWord &r_out, BigWord &r_borrowout) noexcept;
	static BITMATH_CONSTEXPR void bits_Mul(BigWord p_x, BigWord p_y, BigWord &r_hi, BigWord &r_lo) noexcept;
	static BITMATH_CONSTEXPR void bits_Div(BigWord p_hi, BigWord p_lo, BigWord p_y, BigWord &r_quo, BigWord &r_rem) noexcept;
#undef BITMATH_CONSTEXPR

	// arith.go
	static void mulWW(BigWord p_x, BigWord p_y, BigWord &r_z1, BigWord &r_z0) noexcept;
	static void mulAddWWW(BigWord p_x, BigWord p_y, BigWord p_c, BigWord &r_z1, BigWord &r_z0) noexcept;
	[[nodiscard]] static int nlz(BigWord p_x) noexcept;
	static BigWord addVV(Slice r_z, const CSlice &p_x, const CSlice &p_y);
	static BigWord subVV(Slice r_z, const CSlice &p_x, const CSlice &p_y);
	static BigWord addVW(Slice r_z, const CSlice &p_x, BigWord p_y);
	static BigWord addVW_ref(Slice r_z, const CSlice &p_x, BigWord p_y);
	static BigWord subVW(Slice r_z, const CSlice &p_x, BigWord p_y);
	static BigWord subVW_ref(Slice r_z, const CSlice &p_x, BigWord p_y);
	static BigWord lshVU(Slice r_z, const CSlice &p_x, uint64_t p_s);
	static BigWord rshVU(Slice r_z, const CSlice &p_x, uint64_t p_s);
	static BigWord mulAddVWW(Slice r_z, const CSlice &p_x, BigWord p_y, BigWord p_r);
	static BigWord addMulVVWW(Slice r_z, const CSlice &p_x, const CSlice &p_y, BigWord p_m, BigWord p_a);
	static void divWW(BigWord p_x1, BigWord p_x0, BigWord p_y, BigWord p_m, BigWord &r_q, BigWord &r_r);
	[[nodiscard]] static BigWord reciprocalWord(BigWord p_d1);

	// float.go
	int64_t fnorm();
	[[nodiscard]] uint32_t msb32() const;
	[[nodiscard]] uint64_t msb64() const;

	// int.go
	[[nodiscard]] uint32_t low32() const;
	[[nodiscard]] uint64_t low64() const;
	godot::Error modInverse(const BigNat &p_g, const BigNat &p_n);

	// intconv.go
	static void scanSign(godot::String &s, bool &neg);

	// nat.go
	[[nodiscard]] godot::String String() const;
	void norm();
	void setUint64(uint64_t p_x);
	void add(BigNat p_x, BigNat p_y);
	void add1(BigNat p_x);
	void sub(BigNat p_x, BigNat p_y);
	void sub1(BigNat p_x);
	[[nodiscard]] int cmp(const BigNat &p_y) const;
	void montgomery(BigNat p_x, BigNat p_y, BigNat p_m, BigWord p_k, int64_t p_n);
	static void addTo(Slice r_z, const CSlice &p_x);
	void mulRange(uint64_t p_a, uint64_t p_b);
	[[nodiscard]] int64_t bitLen() const;
	[[nodiscard]] uint64_t trailingZeroBits() const;
	[[nodiscard]] bool isPow2(uint64_t &r_p) const;
	void lsh(BigNat p_x, uint64_t p_s);
	void rsh(BigNat p_x, uint64_t p_s);
	void setBit(BigNat p_x, uint64_t p_i, uint64_t p_b);
	[[nodiscard]] uint64_t bit(uint64_t p_i) const;
	[[nodiscard]] uint64_t sticky(uint64_t p_i) const;
	void and_(BigNat p_x, BigNat p_y);
	void trunc(BigNat p_x, uint64_t p_n);
	void andNot(BigNat p_x, BigNat p_y);
	void or_(BigNat p_x, BigNat p_y);
	void xor_(BigNat p_x, BigNat p_y);
	void random(const std::function<uint32_t()> &p_rand, BigNat p_limit, int64_t p_n);
	void expNN(BigNat p_x, BigNat p_y, BigNat p_m, bool p_slow);
	void expNNMontgomeryEven(const BigNat &p_x, const BigNat &p_y, const BigNat &p_m);
	void expNNWindowed(const BigNat &p_x, const BigNat &p_y, uint64_t p_logM);
	void expNNMontgomery(const BigNat &p_x, const BigNat &p_y, const BigNat &p_m);
	[[nodiscard]] int64_t bytes(godot::PackedByteArray &r_buf) const;
	void setBytes(const godot::PackedByteArray &p_buf);
	void sqrt(BigNat p_x);
	void subMod2N(BigNat p_x, BigNat p_y, uint64_t p_n);

	// natconv.go
	static constexpr int64_t MAX_BASE = 10 + ('z' - 'a' + 1) + ('Z' - 'A' + 1);
	[[nodiscard]] static int64_t test_override_leaf_size_bind(int64_t p_leaf_size);
	[[nodiscard]] static BigWord pow(BigWord x, int64_t n);
	[[nodiscard]] godot::Error scan(godot::String &s, int64_t &base, bool fracOk, int64_t &fcount);
	[[nodiscard]] godot::PackedByteArray utoa(int64_t p_base) const;
	[[nodiscard]] godot::PackedByteArray itoa(bool p_neg, int64_t p_base) const;
	void convertWords(uint8_t *s, int64_t slen, BigWord p_b, int64_t p_ndigits, BigWord p_bb, const godot::Span<BigDivisor> &p_table);
	void expWW(BigWord p_x, BigWord p_y);
	static void divisors(int64_t p_m, BigWord p_b, int64_t p_ndigits, BigWord p_bb, godot::LocalVector<BigDivisor> &r_table);

	// natdiv.go
	[[nodiscard]] static godot::TypedArray<godot::PackedInt64Array> test_div_bind(uint64_t p_div_recursive_threshold, const godot::PackedInt64Array &p_u, const godot::PackedInt64Array &p_v);
	void rem(BigNat p_u, BigNat p_v);
	void div(BigNat p_u, BigNat p_v, BigNat &r_r);
	[[nodiscard]] BigWord divW(BigNat p_x, BigWord p_y);
	[[nodiscard]] BigWord modW(BigWord p_d) const;
	[[nodiscard]] static BigWord divWVW(Slice r_z, BigWord p_xn, const CSlice &p_x, BigWord p_y);
	void divLarge(BigNat p_u, BigNat p_v, BigNat &r_r);
	static void divBasic(Slice r_z, Slice r_u, const CSlice &p_v);
	[[nodiscard]] static bool greaterThan(BigWord p_x1, BigWord p_x0, BigWord p_y1, BigWord p_y0);
	static void divRecursive(Slice r_z, Slice r_u, const CSlice &p_v);
	static void divRecursiveStep(Slice r_z, Slice r_u, const CSlice &p_v);

	// natmul.go
	[[nodiscard]] static godot::PackedInt64Array test_mul_bind(uint64_t p_karatsuba_threshold, const godot::PackedInt64Array &p_x, const godot::PackedInt64Array &p_y);
	[[nodiscard]] static godot::PackedInt64Array test_sqr_bind(uint64_t p_basic_sqr_threshold, uint64_t p_karatsuba_sqr_threshold, const godot::PackedInt64Array &p_x);
	void mul(BigNat p_x, BigNat p_y);
	void sqr(BigNat p_x);
	static void basicSqr(Slice r_z, const CSlice &p_x);
	void mulAddWW(BigNat p_x, BigWord p_y, BigWord p_r);
	static void basicMul(Slice r_z, const CSlice &p_x, const CSlice &p_y);
	static void karatsuba(Slice r_z, const CSlice &p_x, const CSlice &p_y);
	static void karatsubaSqr(Slice r_z, const CSlice &p_x);

	// prime.go
	[[nodiscard]] bool probablyPrimeMillerRabin(int64_t p_reps, bool p_force2) const;
	[[nodiscard]] bool probablyPrimeLucas() const;

	// rat.go
	static bool quotToFloat32(const BigNat &p_a, const BigNat &p_b, float &r_f);
	static bool quotToFloat64(const BigNat &p_a, const BigNat &p_b, double &r_f);

	// ratconv.go
	[[nodiscard]] static godot::Error scanExponent(godot::String &s, bool base2ok, bool sepOk, int64_t &exp, int64_t &base);

	// godot_big_varint.cpp
	[[nodiscard]] godot::PackedByteArray to_uvarint() const;
	[[nodiscard]] uint64_t from_uvarint(const godot::Span<uint8_t> &p_bytes);

#ifdef DEBUG_ENABLED
	static inline bool is_test_suite = false;
#else
	static constexpr bool is_test_suite = false;
#endif
};

struct BigDivisor {
	BigNat bbb; // divisor
	int64_t nbits = 0; // bit length of divisor (discounting leading zeros) ~= log2(bbb)
	int64_t ndigits = 0; // digit length of divisor in terms of output base digits
};

#ifdef _MSC_VER
#include <immintrin.h>
void BigNat::bits_Add(BigWord p_x, BigWord p_y, BigWord p_carry, BigWord &r_out, BigWord &r_carryout) noexcept {
	uint8_t carryout1, carryout2;
	carryout1 = _addcarry_u64(0, p_x, p_y, &r_out);
	carryout2 = _addcarry_u64(0, r_out, p_carry, &r_out);
	r_carryout = static_cast<BigWord>(carryout1 + carryout2);
}
void BigNat::bits_Sub(BigWord p_x, BigWord p_y, BigWord p_borrow, BigWord &r_out, BigWord &r_borrowout) noexcept {
	uint8_t borrowout1, borrowout2;
	borrowout1 = _subborrow_u64(0, p_x, p_y, &r_out);
	borrowout2 = _subborrow_u64(0, r_out, p_borrow, &r_out);
	r_borrowout = static_cast<BigWord>(borrowout1 + borrowout2);
}
void BigNat::bits_Mul(BigWord p_x, BigWord p_y, BigWord &r_hi, BigWord &r_lo) noexcept {
	r_lo = _umul128(p_x, p_y, &r_hi);
}
void BigNat::bits_Div(BigWord p_hi, BigWord p_lo, BigWord p_y, BigWord &r_quo, BigWord &r_rem) noexcept {
	r_quo = _udiv128(p_hi, p_lo, p_y, &r_rem);
}
#else
constexpr void BigNat::bits_Add(BigWord p_x, BigWord p_y, BigWord p_carry, BigWord &r_out, BigWord &r_carryout) noexcept {
#if __WORDSIZE == 64 && __has_builtin(__builtin_addcl)
	static_assert(sizeof(BigWord) == sizeof(unsigned long));
	r_out = __builtin_addcl(p_x, p_y, p_carry, &r_carryout);
#elif __WORDSIZE == 32 && __has_builtin(__builtin_addcll)
	static_assert(sizeof(BigWord) == sizeof(unsigned long long));
	r_out = __builtin_addcll(p_x, p_y, p_carry, &r_carryout);
#else
	r_out = p_x + p_y + p_carry;
	// The sum will overflow if both top bits are set (x & y) or if one of them
	// is (x | y), and a carry from the lower place happened. If such a carry
	// happens, the top bit will be 1 + 0 + 1 = 0 (&^ sum).
	r_carryout = ((p_x & p_y) | ((p_x | p_y) & ~r_out)) >> 63;
#endif
}
constexpr void BigNat::bits_Sub(BigWord p_x, BigWord p_y, BigWord p_borrow, BigWord &r_out, BigWord &r_borrowout) noexcept {
#if __WORDSIZE == 64 && __has_builtin(__builtin_subcl)
	static_assert(sizeof(BigWord) == sizeof(unsigned long));
	r_out = __builtin_subcl(p_x, p_y, p_borrow, &r_borrowout);
#elif __WORDSIZE == 32 && __has_builtin(__builtin_subcll)
	static_assert(sizeof(BigWord) == sizeof(unsigned long long));
	r_out = __builtin_subcll(p_x, p_y, p_borrow, &r_borrowout);
#else
	r_out = p_x - p_y - p_borrow;
	// The difference will underflow if the top bit of x is not set and the top
	// bit of y is set (^x & y) or if they are the same (^(x ^ y)) and a borrow
	// from the lower place happens. If that borrow happens, the result will be
	// 1 - 1 - 1 = 0 - 0 - 1 = 1 (& diff).
	r_borrowout = ((~p_x & p_y) | (~(p_x ^ p_y) & r_out)) >> 63;
#endif
}
constexpr void BigNat::bits_Mul(BigWord p_x, BigWord p_y, BigWord &r_hi, BigWord &r_lo) noexcept {
	const unsigned __int128 product = static_cast<unsigned __int128>(p_x) * static_cast<unsigned __int128>(p_y);
	r_hi = static_cast<BigWord>(product >> 64);
	r_lo = static_cast<BigWord>(product);
}
constexpr void BigNat::bits_Div(BigWord p_hi, BigWord p_lo, BigWord p_y, BigWord &r_quo, BigWord &r_rem) noexcept {
	const unsigned __int128 x = (static_cast<unsigned __int128>(p_hi) << 64) | static_cast<unsigned __int128>(p_lo);
	r_quo = static_cast<BigWord>(x / p_y);
	r_rem = static_cast<BigWord>(x % p_y);
}
#endif
