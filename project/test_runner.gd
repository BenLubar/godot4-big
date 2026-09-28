class_name TestRunner
extends Control

@export var suite_scripts: Array[Script]

class SuiteFunction:
	func _init(s: TestSuite, n: String) -> void:
		suite = s
		name = n
	static func compare_names(a: SuiteFunction, b: SuiteFunction) -> bool:
		return a.name < b.name
	# immutable
	var suite: TestSuite
	var name: String
	var errors: PackedStringArray
	var tree_item: TreeItem
	# protected by _mutex
	var test_result: TestingT
	var example_result: TestingE
	var benchmark_result: TestingB

var _suites: Array[TestSuite]
var _queued_tests: Array[SuiteFunction]
var _queued_examples: Array[SuiteFunction]
var _queued_benchmarks: Array[SuiteFunction]

var _mutex := Mutex.new()
var _tests_and_examples_task := -1
var _benchmark_iterator := -1
var _benchmark_task := -1

@onready var _tree: Tree = %Tree

func _ready() -> void:
	_suites.append_array(suite_scripts.map(func(s: Script) -> TestSuite: return s.new()))

	for suite in _suites:
		for fn in suite.get_test_names():
			_queued_tests.append(SuiteFunction.new(suite, fn))
		for fn in suite.get_example_names():
			_queued_examples.append(SuiteFunction.new(suite, fn))
		for fn in suite.get_benchmark_names():
			_queued_benchmarks.append(SuiteFunction.new(suite, fn))

	_queued_tests.sort_custom(SuiteFunction.compare_names)
	_queued_examples.sort_custom(SuiteFunction.compare_names)
	_queued_benchmarks.sort_custom(SuiteFunction.compare_names)

	var true_root := _tree.create_item()

	var tests_root := true_root.create_child()
	tests_root.set_text(0, "Tests")
	tests_root.set_selectable(0, false)
	for queued in _queued_tests:
		queued.tree_item = tests_root.create_child()
		queued.tree_item.set_text(0, queued.name)
		queued.tree_item.set_text(1, tr(&"..."))
		queued.tree_item.set_selectable(1, false)
	for queued in _queued_examples:
		queued.tree_item = tests_root.create_child()
		queued.tree_item.set_text(0, queued.name)
		queued.tree_item.set_text(1, tr(&"..."))
		queued.tree_item.set_selectable(1, false)

	var benchmarks_root := true_root.create_child()
	benchmarks_root.set_text(0, "Benchmarks")
	benchmarks_root.set_selectable(0, false)
	for queued in _queued_benchmarks:
		queued.tree_item = benchmarks_root.create_child()
		queued.tree_item.set_text(0, queued.name)
		queued.tree_item.set_text(1, tr(&"..."))
		queued.tree_item.set_selectable(1, false)

	if false:
		for i in len(_queued_tests) + len(_queued_examples):
			_run_test_or_example(i)
		for i in len(_queued_benchmarks):
			_run_benchmark(i)
	else:
		_tests_and_examples_task = WorkerThreadPool.add_group_task(_run_test_or_example, len(_queued_tests) + len(_queued_examples), -1, true, "run tests and examples")

func _process(_delta: float) -> void:
	if _tests_and_examples_task != -1 and WorkerThreadPool.is_group_task_completed(_tests_and_examples_task):
		WorkerThreadPool.wait_for_group_task_completion(_tests_and_examples_task)
		_tests_and_examples_task = -1
		assert(_benchmark_task == -1)
		if not _queued_benchmarks.is_empty():
			assert(_benchmark_iterator == -1)
			_benchmark_iterator = 0
			_benchmark_task = WorkerThreadPool.add_task(_run_benchmark.bind(_benchmark_iterator), true, "benchmark %s" % [_queued_benchmarks[_benchmark_iterator].name])
	elif _benchmark_task != -1 and WorkerThreadPool.is_task_completed(_benchmark_task):
		WorkerThreadPool.wait_for_task_completion(_benchmark_task)
		if _benchmark_iterator == -1:
			# it was a rerun
			_benchmark_task = -1
		elif _benchmark_iterator + 1 >= len(_queued_benchmarks):
			# we've reached the end of the list
			_benchmark_iterator = -1
			_benchmark_task = -1
		else:
			_benchmark_iterator += 1
			_benchmark_task = WorkerThreadPool.add_task(_run_benchmark.bind(_benchmark_iterator), true, "benchmark %s" % [_queued_benchmarks[_benchmark_iterator].name])

func _run_test_or_example(i: int) -> void:
	if i < len(_queued_tests):
		var test := _queued_tests[i]
		var result := test.suite.run_test(test.name)
		_mutex.lock()
		test.test_result = result
		_mutex.unlock()
		_on_test_finished.call_deferred(i)
	else:
		var example := _queued_examples[i - len(_queued_tests)]
		var result := example.suite.run_example(example.name)
		_mutex.lock()
		example.example_result = result
		_mutex.unlock()
		_on_example_finished.call_deferred(i - len(_queued_tests))

func _run_benchmark(i: int) -> void:
	var benchmark := _queued_benchmarks[i]
	var result := benchmark.suite.run_benchmark(benchmark.name)
	_mutex.lock()
	benchmark.benchmark_result = result
	_mutex.unlock()
	_on_benchmark_finished.call_deferred(i)

func _on_test_finished(i: int) -> void:
	var test := _queued_tests[i]
	_mutex.lock()
	if test.test_result.failed:
		test.tree_item.set_text(1, tr(&"FAIL"))
		test.tree_item.set_custom_color(1, Color.RED)
		if test.test_result.errors == PackedStringArray(["TODO"]):
			test.tree_item.set_text(1, "TODO")
			test.tree_item.set_custom_color(1, Color.YELLOW)
	else:
		test.tree_item.set_text(1, tr(&"PASS"))
		test.tree_item.set_custom_color(1, Color.GREEN)
	_mutex.unlock()
	# TODO

func _on_example_finished(i: int) -> void:
	var example := _queued_examples[i]
	_mutex.lock()
	if example.example_result.failed:
		example.tree_item.set_text(1, tr(&"FAIL"))
		example.tree_item.set_custom_color(1, Color.RED)
	else:
		example.tree_item.set_text(1, tr(&"PASS"))
		example.tree_item.set_custom_color(1, Color.GREEN)
	_mutex.unlock()
	# TODO

func _on_benchmark_finished(i: int) -> void:
	var _benchmark := _queued_benchmarks[i]
	# TODO
