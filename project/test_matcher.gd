## This file is ported from src/testing/match.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2015 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

## matcher sanitizes, uniques, and filters names of subtests and subbenchmarks.
class_name TestMatcher

enum MatchType {
	NO_MATCH = 0,
	PARTIAL_MATCH = 1,
	FULL_MATCH = 2,
}

## matches checks the name against the receiver's pattern strings using the
## given match function.
func matches(_name: PackedStringArray) -> MatchType:
	return MatchType.NO_MATCH

## simpleMatch matches a test name if all of the pattern strings match in
## sequence.
class simpleMatch extends TestMatcher:
	var patterns: Array[RegEx]
	func matches(name: PackedStringArray) -> MatchType:
		for i in len(name):
			if i >= len(patterns):
				break
			if not patterns[i].search(name[i]):
				return MatchType.NO_MATCH
		if len(name) < len(patterns):
			return MatchType.PARTIAL_MATCH
		return MatchType.FULL_MATCH

## alternationMatch matches a test name if one of the alternations match.
class alternationMatch extends TestMatcher:
	var alternations: Array[TestMatcher]
	func matches(name: PackedStringArray) -> MatchType:
		for m in alternations:
			var ok := m.matches(name)
			if ok != MatchType.NO_MATCH:
				return ok
		return MatchType.NO_MATCH

static func splitRegexp(s: String) -> TestMatcher:
	var a := simpleMatch.new()
	var b := alternationMatch.new()
	var cs := 0
	var cp := 0
	var i := 0
	while i < len(s):
		match s[i]:
			'[':
				cs += 1
			']':
				cs -= 1
				if cs < 0: ## An unmatched ']' is legal.
					cs = 0
			'(':
				if cs == 0:
					cp += 1
			')':
				if cs == 0:
					cp -= 1
			'\\':
				i += 1
			'/':
				if cs == 0 and cp == 0:
					a.patterns.append(RegEx.create_from_string(s.substr(0, i)))
					s = s.substr(i + 1)
					i = 0
					continue
			'|':
				if cs == 0 and cp == 0:
					a.patterns.append(RegEx.create_from_string(s.substr(0, i)))
					s = s.substr(i + 1)
					i = 0
					b.alternations.append(a)
					a = simpleMatch.new()
					continue
		i += 1

	a.patterns.append(RegEx.create_from_string(s))
	if b.is_empty():
		return a

	b.alternations.append(a)
	return b
