class_name TestingT

var name: String
var errors: PackedStringArray
var failed: bool:
	get:
		return not errors.is_empty()
var skipped: bool
var sub_tests: Array[TestingT]

func Run(sub_name: String, test_case: Callable) -> void:
	assert(not skipped)
	var sub_t := TestingT.new()
	sub_t.name = name + "/" + sub_name
	sub_tests.append(sub_t)
	if TestSuite.run_matcher and not TestSuite.run_matcher.matches(sub_t.name.split("/")):
		sub_t.Skip()
	else:
		test_case.call(sub_t)
	# TODO

func Error(message: String) -> void:
	assert(not skipped)
	errors.append(message)
	push_error.call_deferred("FAIL %s: %s" % [name, message])
	# TODO

const _num_quick_checks := 100
const _max_quick_bytes := 50

static func _generate_random_bytes(crypto: Crypto) -> PackedByteArray:
	var random_bytes := crypto.generate_random_bytes(4 + _max_quick_bytes - 1)
	var count := random_bytes.decode_u32(0) % _max_quick_bytes
	return random_bytes.slice(4, count + 4)

func QuickCheckB(check: Callable) -> void:
	assert(not skipped)
	var crypto := Crypto.new()
	for i in _num_quick_checks:
		var arg0 := _generate_random_bytes(crypto)
		var ok: bool = check.call(arg0)
		if not ok:
			Error("#%d: failed on input %s" % [i + 1, arg0])

func QuickCheckBB(check: Callable) -> void:
	assert(not skipped)
	var crypto := Crypto.new()
	for i in _num_quick_checks:
		var arg0 := _generate_random_bytes(crypto)
		var arg1 := _generate_random_bytes(crypto)
		var ok: bool = check.call(arg0, arg1)
		if not ok:
			Error("#%d: failed on input %s, %s" % [i + 1, arg0, arg1])

func QuickCheckIIIIIIIII(check: Callable) -> void:
	assert(not skipped)
	var crypto := Crypto.new()
	for i in _num_quick_checks:
		var random_bytes := crypto.generate_random_bytes(8 * 9)
		var arg0 := random_bytes.decode_u64(8 * 0)
		var arg1 := random_bytes.decode_u64(8 * 1)
		var arg2 := random_bytes.decode_u64(8 * 2)
		var arg3 := random_bytes.decode_u64(8 * 3)
		var arg4 := random_bytes.decode_u64(8 * 4)
		var arg5 := random_bytes.decode_u64(8 * 5)
		var arg6 := random_bytes.decode_u64(8 * 6)
		var arg7 := random_bytes.decode_u64(8 * 7)
		var arg8 := random_bytes.decode_u64(8 * 8)
		var ok: bool = check.call(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
		if not ok:
			Error("#%d: failed on input %s, %s, %s, %s, %s, %s, %s, %s, %s" % [i + 1, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8])

func Skip() -> void:
	assert(not skipped)
	skipped = true
