class_name TestingT

var name: String
var errors: PackedStringArray
var failed: bool:
	get:
		return not errors.is_empty()

func Run(_sub_name: String, test_case: Callable) -> void:
	test_case.call(self)
	# TODO

func Error(message: String) -> void:
	errors.append(message)
	push_error.call_deferred("FAIL %s: %s" % [name, message])
	# TODO

const _num_quick_checks := 100
const _max_quick_bytes := 50

func _generate_random_bytes() -> PackedByteArray:
	var crypto := Crypto.new()
	var random_bytes := crypto.generate_random_bytes(4 + _max_quick_bytes - 1)
	var count := random_bytes.decode_u32(0) % _max_quick_bytes
	return random_bytes.slice(4, count + 4)

func QuickCheckB(_check: Callable) -> void:
	for i in _num_quick_checks:
		var arg0 := _generate_random_bytes()
		var ok: bool = _check.call(arg0)
		if not ok:
			Error("#%d: failed on input %s" % [i + 1, arg0])

func QuickCheckBB(_check: Callable) -> void:
	for i in _num_quick_checks:
		var arg0 := _generate_random_bytes()
		var arg1 := _generate_random_bytes()
		var ok: bool = _check.call(arg0, arg1)
		if not ok:
			Error("#%d: failed on input %s, %s" % [i + 1, arg0, arg1])
