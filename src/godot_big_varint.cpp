#include "godot_big_naturals.h"
#include "godot_big_int.h"
#include "godot_big_float.h"
#include "godot_big_rat.h"

#include <bit>

using namespace godot;

PackedByteArray BigNat::to_uvarint() const {
	if (array.is_empty()) {
		return PackedByteArray{0};
	}

	BigWord upper = 0;
	BigWord lower = 0;
	int64_t bits_remaining = 0;
	PackedByteArray bytes;
	for (int64_t i = 0; i < array.size() - 1; i++) {
		const BigWord w = array[i];
		lower |= w << bits_remaining;
		upper |= w >> bits_remaining;
		bits_remaining += 64;

		while (bits_remaining > 7) {
			bytes.append(uint8_t(lower | 128));
			lower >>= 7;
			lower |= upper << (64 - 7);
			upper >>= 7;
			bits_remaining -= 7;
		}
	}

	const BigWord w = array[array.size() - 1];
	lower |= w << bits_remaining;
	upper |= w >> bits_remaining;
	bits_remaining += 64 - std::countl_zero(w);

	while (bits_remaining > 7) {
		bytes.append(uint8_t(lower | 128));
		lower >>= 7;
		lower |= upper << (64 - 7);
		upper >>= 7;
		bits_remaining -= 7;
	}

	DEV_ASSERT(upper == 0 && lower < 128);
	bytes.append(lower);

	return bytes;
}

uint64_t BigNat::from_uvarint(const Span<uint8_t> &p_bytes) {
	BigNat total, current;
	for (uint64_t advance = 0; advance < p_bytes.size(); advance++) {
		current.setUint64(p_bytes[advance] & 0x7f);
		current.lsh(current, advance * 7);
		total.add(total, current);
		if (!(p_bytes[advance] & 0x80)) {
			array = total.array;
			return advance + 1;
		}
	}

	// ran out of bytes
	return 0;
}

PackedByteArray BigInt::to_uvarint() const {
	return _abs.to_uvarint();
}

int64_t BigInt::from_uvarint(const PackedByteArray &p_bytes, int64_t p_offset) {
	BigNat abs;
	const uint64_t advance = abs.from_uvarint(Span(p_bytes.ptr() + p_offset, p_bytes.size() - p_offset));
	if (advance != 0 && (_neg || _abs.array != abs.array)) {
		_neg = false;
		_abs.array = abs.array;
		emit_changed();
	}
	return advance;
}

PackedByteArray BigInt::to_svarint() const {
	BigNat zigzag;
	zigzag.lsh(_abs, 1);
	if (_neg) {
		zigzag.sub(zigzag, BigNat{{1}});
	}
	return zigzag.to_uvarint();
}

int64_t BigInt::from_svarint(const PackedByteArray &p_bytes, int64_t p_offset) {
	BigNat abs;
	const uint64_t advance = abs.from_uvarint(Span(p_bytes.ptr() + p_offset, p_bytes.size() - p_offset));
	if (advance != 0) {
		const bool neg = !abs.array.is_empty() && (abs[0] & 1) != 0;
		abs.rsh(abs, 1);
		if (neg) {
			abs.add(abs, BigNat{{1}});
		}

		if (_neg != neg || _abs.array != abs.array) {
			_neg = neg;
			_abs.array = abs.array;
			emit_changed();
		}
	}
	return advance;
}

PackedByteArray BigFloat::to_bytes() const {
#ifdef DBGFLAG_ASSERT
	_validate();
#endif

	int64_t mantissa_len = 0;
	PackedByteArray mantissa_len_encoded;
	if (_form == FORM_FINITE) {
		mantissa_len = _mant.array.size();
		// size cannot be negative (we would have run out of virtual memory space) or zero (invalid float)
		BigNat n{{mantissa_len}};
		mantissa_len_encoded = n.to_uvarint();
	}

	PackedByteArray b;
	b.resize(1 + 4 + (_form == FORM_FINITE ? 4 + mantissa_len_encoded.size() + mantissa_len * 8 : 0));
	// first byte is packed with _mode, _acc+1, _form, and _neg as bits: mmmaaffn
	b[0] = ((_mode & 7) << 5) | (((_acc + 1) & 3) << 3) | ((_form & 3) << 1) | (_neg ? 1 : 0);
	// next four bytes are precision (little endian)
	b.encode_u32(1, _prec);

	// if we're non-finite, we're done.
	if (_form != FORM_FINITE) {
		return b;
	}

	// otherwise, exponent (little endian, four bytes) and mantissa
	b.encode_s32(1 + 4, _exp);

	// add our encoded mantissa length and then our raw mantissa (little endian, 8 byte chunks which are also little endian)
	memcpy(b.ptrw() + 1 + 4 + 4, mantissa_len_encoded.ptr(), mantissa_len_encoded.size());
	int64_t offset = 1 + 4 + 4 + mantissa_len_encoded.size();
	for (int64_t i = 0; i < _mant.array.size(); i++) {
		b.encode_u64(offset, _mant[i]);
		offset += 8;
	}
	DEV_ASSERT(offset == b.size());
	return b;
}

int64_t BigFloat::from_bytes(const PackedByteArray &p_bytes, int64_t p_offset) {
	if (p_bytes.size() < p_offset + 5) {
		// cannot possibly fit a BigFloat in here
		return 0;
	}

	// first byte is packed with _mode, _acc+1, _form, and _neg as bits: mmmaaffn
	const uint8_t first_byte = p_bytes[p_offset];
	ERR_FAIL_COND_V_MSG(((first_byte >> 5) & 7) > 5, 0, "invalid rounding mode");
	const RoundingMode mode = static_cast<RoundingMode>((first_byte >> 5) & 7);
	ERR_FAIL_COND_V_MSG(((first_byte >> 3) & 3) > 2, 0, "invalid accuracy");
	const BigAccuracy acc = static_cast<BigAccuracy>(int((first_byte >> 3) & 3) - 1);
	ERR_FAIL_COND_V_MSG(((first_byte >> 1) & 3) > 2, 0, "invalid form");
	const Form form = static_cast<Form>((first_byte >> 1) & 3);
	const bool neg = (first_byte & 1) == 1;

	// next four bytes are precision
	const uint32_t prec = p_bytes.decode_u32(p_offset + 1);

	// if we're non-finite, we're done
	if (form != FORM_FINITE) {
		_mode = mode;
		_acc = acc;
		_form = form;
		_neg = neg;
		_prec = prec;

#ifdef DBGFLAG_ASSERT
		_validate();
#endif

		emit_changed();

		return 5;
	}

	int64_t advance = 1 + 4 + 4;
	BigNat mantissa_len;
	const uint64_t mantissa_len_advance = mantissa_len.from_uvarint(Span(p_bytes.ptr() + p_offset + advance, p_bytes.size() - p_offset - advance));
	ERR_FAIL_COND_V_MSG(mantissa_len_advance == 0 || mantissa_len.array.size() != 1, 0, "invalid mantissa length");
	advance += mantissa_len_advance;
	ERR_FAIL_COND_V_MSG(p_bytes.size() < p_offset + advance + mantissa_len[0] * 8, 0, "buffer ends during mantissa");

	const int32_t exp = p_bytes.decode_s32(p_offset + 1 + 4);

	BigNat mant;
	mant.array.resize(mantissa_len[0]);

	for (uint64_t i = 0; i < mantissa_len[0]; i++) {
		mant[i] = p_bytes.decode_u64(p_offset + advance);
		advance += 8;
	}

	// check what _validate would check on debug builds here as well before we put the data in
	constexpr BigWord msb = 1LLU << (64 - 1);
	ERR_FAIL_COND_V_MSG((mant[mantissa_len[0] - 1] & msb) == 0, 0, "msb not set in last word");
	ERR_FAIL_COND_V_MSG(prec == 0, 0, "zero precision finite number");

	_mode = mode;
	_acc = acc;
	_form = form;
	_neg = neg;
	_prec = prec;
	_exp = exp;
	_mant = mant;

#ifdef DBGFLAG_ASSERT
	_validate();
#endif

	return advance;
}

// { svarint numerator; uvarint denominator }
PackedByteArray BigRat::to_bytes() const {
	Ref<BigInt> n;
	n.instantiate();
	n->_neg = _neg;
	n->_abs = _a;

	const PackedByteArray numerator = n->to_svarint();

	n->_abs = _b;

	const PackedByteArray denominator = n->to_uvarint();

	return numerator + denominator;
}
int64_t BigRat::from_bytes(const PackedByteArray &p_bytes, int64_t p_offset) {
	Ref<BigInt> n;
	n.instantiate();

	const int64_t advance_num = n->from_svarint(p_bytes, p_offset);
	if (unlikely(advance_num == 0)) {
		return 0;
	}

	const bool neg = n->_neg;
	const BigNat a = n->_abs;

	const int64_t advance_denom = n->from_uvarint(p_bytes, p_offset + advance_num);
	if (unlikely(advance_denom == 0)) {
		return 0;
	}

	// denominator cannot be 0
	if (n->_abs.array.is_empty()) {
		return 0;
	}

	_neg = neg;
	_a = a;
	_b = n->_abs;
	emit_changed();

	return advance_num + advance_denom;
}
