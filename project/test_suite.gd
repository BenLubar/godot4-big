class_name TestSuite

func get_test_names() -> PackedStringArray:
	var names: PackedStringArray
	for m in get_method_list():
		var name: String = m[&"name"]
		if name.begins_with("Test"):
			names.append(name)
	return names

func get_example_names() -> PackedStringArray:
	var names: PackedStringArray
	for m in get_method_list():
		var name: String = m[&"name"]
		if name.begins_with("Example"):
			names.append(name)
	return names

func get_benchmark_names() -> PackedStringArray:
	var names: PackedStringArray
	for m in get_method_list():
		var name: String = m[&"name"]
		if name.begins_with("Benchmark"):
			names.append(name)
	return names

func run_test(name: String) -> TestingT:
	var t := TestingT.new()
	t.name = name
	call(name, t)
	return t

func run_example(name: String) -> TestingE:
	var e := TestingE.new()
	e.name = name
	e.expected_output = get(name + "_expected_output")
	call(name, e)
	if e.failed:
		push_error("EXAMPLE FAIL %s\nwant: %s\ngot : %s" % [name, e.expected_output, e._output])
	return e

func run_benchmark(name: String) -> TestingB:
	var b := TestingB.new()
	b.name = name
	b._run1(Callable(self, name))
	return b

static var short := false
static var long := false
static var calibrate := false
static var run_matcher: TestMatcher
