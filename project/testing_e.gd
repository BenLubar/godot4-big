class_name TestingE

var _output: PackedStringArray
var expected_output: PackedStringArray

func Output(message: Variant) -> void:
	_output.append(str(message))
