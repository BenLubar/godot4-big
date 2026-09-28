class_name TestingE

var name: String
var _output: PackedStringArray
var expected_output: PackedStringArray
var failed: bool:
	get:
		return _output == expected_output

func Output(message: Variant) -> void:
	_output.append(str(message))
