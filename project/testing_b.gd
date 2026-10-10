class_name TestingB

var name: String
var N: int
var skipped: bool
var sub_benchmarks: Array[TestingB]

var _skip_time: int
var _stopped_at: int
var _started_at: int
var _total_time: int
var _bytes: int
var _report_allocs: bool

func Run(sub_name: String, benchmark: Callable) -> void:
	assert(not skipped)
	assert(N == 1)
	var sub_b := TestingB.new()
	sub_b.name = name + "/" + sub_name
	sub_benchmarks.append(sub_b)
	if TestSuite.run_matcher and not TestSuite.run_matcher.matches(sub_b.name.split("/")):
		sub_b.Skip()
	else:
		sub_b._run1(benchmark)
	# TODO

const goalUS := 1000000

func _run1(benchmark: Callable) -> void:
	assert(not skipped)
	assert(sub_benchmarks.is_empty())
	N = 1
	while true:
		print("trying benchmark ", name, " with iterations ", N)
		_skip_time = 0
		_stopped_at = 0

		_started_at = Time.get_ticks_usec()

		benchmark.call(self)

		_total_time = Time.get_ticks_usec() - _started_at - _skip_time
		assert(_stopped_at == 0)

		if not sub_benchmarks.is_empty():
			assert(N == 1)
			break

		if _total_time >= goalUS:
			break

		var nextN := _predictN(N, _total_time)
		if N >= nextN:
			break
		N = nextN

static func _predictN(prevN: int, prevUS: int) -> int:
	## This function is ported from src/testing/benchmark.go in Go 1.27.1.
	## Original copyright notice follows:

	## Copyright 2009 The Go Authors. All rights reserved.
	## Use of this source code is governed by a BSD-style
	## license that can be found in the LICENSE file.

	## Don't run more than 1e9 times. (This also keeps n in int range on 32 bit platforms.)
	const maxBenchPredictIters := 1000000000

	## Round up to dodge divide by zero. See https://go.dev/issue/70709.
	prevUS = maxi(prevUS, 1)

	## Order of operations matters.
	## For very fast benchmarks, prevIters ~= prevns.
	## If you divide first, you get 0 or 1,
	## which can hide an order of magnitude in execution time.
	## So multiply first, then divide.
	@warning_ignore("integer_division")
	var n := goalUS * prevN / prevUS
	## Run more iterations than we think we'll need (1.2x).
	@warning_ignore("integer_division")
	n += n / 5
	## Don't grow too fast in case we had timing errors previously.
	n = mini(n, 100 * prevN)
	## Be sure to run at least one more than last time.
	n = maxi(n, prevN + 1)
	## Don't run more than 1e9 times. (This also keeps n in int range on 32 bit platforms.)
	n = mini(n, maxBenchPredictIters)
	return n

func StopTimer() -> void:
	assert(not skipped)
	assert(_stopped_at == 0)
	_stopped_at = Time.get_ticks_usec()

func StartTimer() -> void:
	assert(not skipped)
	assert(_stopped_at != 0)
	_skip_time += Time.get_ticks_usec() - _stopped_at
	_stopped_at = 0

func ResetTimer() -> void:
	assert(not skipped)
	assert(_stopped_at == 0)
	_skip_time = 0
	_started_at = Time.get_ticks_usec()

func SetBytes(n: int) -> void:
	assert(not skipped)
	_bytes = n

func ReportAllocs() -> void:
	assert(not skipped)
	_report_allocs = true

func Skip() -> void:
	assert(not skipped)
	assert(N == 1)
	assert(sub_benchmarks.is_empty())
	skipped = true
