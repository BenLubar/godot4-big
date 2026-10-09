class_name TestingB

var name: String
var N: int
var skipped: bool
var sub_benchmarks: Array[TestingB]

func Run(sub_name: String, benchmark: Callable) -> void:
	assert(not skipped)
	assert(N == 1)
	var sub_b := TestingB.new()
	sub_b.name = name + "/" + sub_name
	sub_b.N = 1
	sub_benchmarks.append(sub_b)
	if TestSuite.run_regexp and not TestSuite.run_regexp.search(sub_b.name):
		sub_b.Skip()
	else:
		benchmark.call(sub_b)
	# TODO

func StopTimer() -> void:
	assert(not skipped)
	pass # TODO

func StartTimer() -> void:
	assert(not skipped)
	pass # TODO

func ReportAllocs() -> void:
	assert(not skipped)
	pass # TODO

func Skip() -> void:
	assert(not skipped)
	assert(N == 1)
	assert(sub_benchmarks.is_empty())
	skipped = true
