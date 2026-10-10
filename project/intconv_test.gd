## This file is ported from src/math/big/intconv_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2015 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

class_name IntConvTest
extends TestSuite

#var stringTests = []struct {
#	in   string
#	out  string
#	base int
#	val  int64
#	ok   bool
#}{
#	// invalid inputs
#	{in: ""},
#	{in: "a"},
#	{in: "z"},
#	{in: "+"},
#	{in: "-"},
#	{in: "0b"},
#	{in: "0o"},
#	{in: "0x"},
#	{in: "0y"},
#	{in: "2", base: 2},
#	{in: "0b2", base: 0},
#	{in: "08"},
#	{in: "8", base: 8},
#	{in: "0xg", base: 0},
#	{in: "g", base: 16},
#
#	// invalid inputs with separators
#	// (smoke tests only - a comprehensive set of tests is in natconv_test.go)
#	{in: "_"},
#	{in: "0_"},
#	{in: "_0"},
#	{in: "-1__0"},
#	{in: "0x10_"},
#	{in: "1_000", base: 10}, // separators are not permitted for bases != 0
#	{in: "d_e_a_d", base: 16},
#
#	// valid inputs
#	{"0", "0", 0, 0, true},
#	{"0", "0", 10, 0, true},
#	{"0", "0", 16, 0, true},
#	{"+0", "0", 0, 0, true},
#	{"-0", "0", 0, 0, true},
#	{"10", "10", 0, 10, true},
#	{"10", "10", 10, 10, true},
#	{"10", "10", 16, 16, true},
#	{"-10", "-10", 16, -16, true},
#	{"+10", "10", 16, 16, true},
#	{"0b10", "2", 0, 2, true},
#	{"0o10", "8", 0, 8, true},
#	{"0x10", "16", 0, 16, true},
#	{in: "0x10", base: 16},
#	{"-0x10", "-16", 0, -16, true},
#	{"+0x10", "16", 0, 16, true},
#	{"00", "0", 0, 0, true},
#	{"0", "0", 8, 0, true},
#	{"07", "7", 0, 7, true},
#	{"7", "7", 8, 7, true},
#	{"023", "19", 0, 19, true},
#	{"23", "23", 8, 19, true},
#	{"cafebabe", "cafebabe", 16, 0xcafebabe, true},
#	{"0b0", "0", 0, 0, true},
#	{"-111", "-111", 2, -7, true},
#	{"-0b111", "-7", 0, -7, true},
#	{"0b1001010111", "599", 0, 0x257, true},
#	{"1001010111", "1001010111", 2, 0x257, true},
#	{"A", "a", 36, 10, true},
#	{"A", "A", 37, 36, true},
#	{"ABCXYZ", "abcxyz", 36, 623741435, true},
#	{"ABCXYZ", "ABCXYZ", 62, 33536793425, true},
#
#	// valid input with separators
#	// (smoke tests only - a comprehensive set of tests is in natconv_test.go)
#	{"1_000", "1000", 0, 1000, true},
#	{"0b_1010", "10", 0, 10, true},
#	{"+0o_660", "432", 0, 0660, true},
#	{"-0xF00D_1E", "-15731998", 0, -0xf00d1e, true},
#}

func TestIntText(t: TestingT) -> void:
	t.Error("TODO")
#	z := new(Int)
#	for _, test := range stringTests {
#		if !test.ok {
#			continue
#		}
#
#		_, ok := z.SetString(test.in, test.base)
#		if !ok {
#			t.Errorf("%v: failed to parse", test)
#			continue
#		}
#
#		base := test.base
#		if base == 0 {
#			base = 10
#		}
#
#		if got := z.Text(base); got != test.out {
#			t.Errorf("%v: got %s; want %s", test, got, test.out)
#		}
#	}
#}

func TestAppendText(t: TestingT) -> void:
	t.Error("TODO")
#	z := new(Int)
#	var buf []byte
#	for _, test := range stringTests {
#		if !test.ok {
#			continue
#		}
#
#		_, ok := z.SetString(test.in, test.base)
#		if !ok {
#			t.Errorf("%v: failed to parse", test)
#			continue
#		}
#
#		base := test.base
#		if base == 0 {
#			base = 10
#		}
#
#		i := len(buf)
#		buf = z.Append(buf, base)
#		if got := string(buf[i:]); got != test.out {
#			t.Errorf("%v: got %s; want %s", test, got, test.out)
#		}
#	}
#}
#
#func format(base int) string {
#	switch base {
#	case 2:
#		return "%b"
#	case 8:
#		return "%o"
#	case 16:
#		return "%x"
#	}
#	return "%d"
#}

func TestGetString(t: TestingT) -> void:
	t.Error("TODO")
#	z := new(Int)
#	for i, test := range stringTests {
#		if !test.ok {
#			continue
#		}
#		z.SetInt64(test.val)
#
#		if test.base == 10 {
#			if got := z.String(); got != test.out {
#				t.Errorf("#%da got %s; want %s", i, got, test.out)
#			}
#		}
#
#		f := format(test.base)
#		got := fmt.Sprintf(f, z)
#		if f == "%d" {
#			if got != fmt.Sprintf("%d", test.val) {
#				t.Errorf("#%db got %s; want %d", i, got, test.val)
#			}
#		} else {
#			if got != test.out {
#				t.Errorf("#%dc got %s; want %s", i, got, test.out)
#			}
#		}
#	}
#}

func TestSetString(t: TestingT) -> void:
	t.Error("TODO")
#	tmp := new(Int)
#	for i, test := range stringTests {
#		// initialize to a non-zero value so that issues with parsing
#		// 0 are detected
#		tmp.SetInt64(1234567890)
#		n1, ok1 := new(Int).SetString(test.in, test.base)
#		n2, ok2 := tmp.SetString(test.in, test.base)
#		expected := NewInt(test.val)
#		if ok1 != test.ok || ok2 != test.ok {
#			t.Errorf("#%d (input '%s') ok incorrect (should be %t)", i, test.in, test.ok)
#			continue
#		}
#		if !ok1 {
#			if n1 != nil {
#				t.Errorf("#%d (input '%s') n1 != nil", i, test.in)
#			}
#			continue
#		}
#		if !ok2 {
#			if n2 != nil {
#				t.Errorf("#%d (input '%s') n2 != nil", i, test.in)
#			}
#			continue
#		}
#
#		if ok1 && !isNormalized(n1) {
#			t.Errorf("#%d (input '%s'): %v is not normalized", i, test.in, *n1)
#		}
#		if ok2 && !isNormalized(n2) {
#			t.Errorf("#%d (input '%s'): %v is not normalized", i, test.in, *n2)
#		}
#
#		if n1.Cmp(expected) != 0 {
#			t.Errorf("#%d (input '%s') got: %s want: %d", i, test.in, n1, test.val)
#		}
#		if n2.Cmp(expected) != 0 {
#			t.Errorf("#%d (input '%s') got: %s want: %d", i, test.in, n2, test.val)
#		}
#	}
#}

#struct {
#	input  string
#	format string
#	output string
#}
const formatTests: Array[Array] = [
#	{"<nil>", "%x", "<nil>"},
#	{"<nil>", "%#x", "<nil>"},
#	{"<nil>", "%#y", "%!y(big.Int=<nil>)"},
#
#	{"10", "%b", "1010"},
#	{"10", "%o", "12"},
#	{"10", "%d", "10"},
#	{"10", "%v", "10"},
#	{"10", "%x", "a"},
#	{"10", "%X", "A"},
#	{"-10", "%X", "-A"},
#	{"10", "%y", "%!y(big.Int=10)"},
#	{"-10", "%y", "%!y(big.Int=-10)"},
#
#	{"10", "%#b", "0b1010"},
#	{"10", "%#o", "012"},
#	{"10", "%O", "0o12"},
#	{"-10", "%#b", "-0b1010"},
#	{"-10", "%#o", "-012"},
#	{"-10", "%O", "-0o12"},
#	{"10", "%#d", "10"},
#	{"10", "%#v", "10"},
#	{"10", "%#x", "0xa"},
#	{"10", "%#X", "0XA"},
#	{"-10", "%#X", "-0XA"},
#	{"10", "%#y", "%!y(big.Int=10)"},
#	{"-10", "%#y", "%!y(big.Int=-10)"},
#
#	{"1234", "%d", "1234"},
#	{"1234", "%3d", "1234"},
#	{"1234", "%4d", "1234"},
#	{"-1234", "%d", "-1234"},
#	{"1234", "% 5d", " 1234"},
#	{"1234", "%+5d", "+1234"},
#	{"1234", "%-5d", "1234 "},
#	{"1234", "%x", "4d2"},
#	{"1234", "%X", "4D2"},
#	{"-1234", "%3x", "-4d2"},
#	{"-1234", "%4x", "-4d2"},
#	{"-1234", "%5x", " -4d2"},
#	{"-1234", "%-5x", "-4d2 "},
#	{"1234", "%03d", "1234"},
#	{"1234", "%04d", "1234"},
#	{"1234", "%05d", "01234"},
#	{"1234", "%06d", "001234"},
#	{"-1234", "%06d", "-01234"},
#	{"1234", "%+06d", "+01234"},
#	{"1234", "% 06d", " 01234"},
#	{"1234", "%-6d", "1234  "},
#	{"1234", "%-06d", "1234  "},
#	{"-1234", "%-06d", "-1234 "},
#
#	{"1234", "%.3d", "1234"},
#	{"1234", "%.4d", "1234"},
#	{"1234", "%.5d", "01234"},
#	{"1234", "%.6d", "001234"},
#	{"-1234", "%.3d", "-1234"},
#	{"-1234", "%.4d", "-1234"},
#	{"-1234", "%.5d", "-01234"},
#	{"-1234", "%.6d", "-001234"},
#
#	{"1234", "%8.3d", "    1234"},
#	{"1234", "%8.4d", "    1234"},
#	{"1234", "%8.5d", "   01234"},
#	{"1234", "%8.6d", "  001234"},
#	{"-1234", "%8.3d", "   -1234"},
#	{"-1234", "%8.4d", "   -1234"},
#	{"-1234", "%8.5d", "  -01234"},
#	{"-1234", "%8.6d", " -001234"},
#
#	{"1234", "%+8.3d", "   +1234"},
#	{"1234", "%+8.4d", "   +1234"},
#	{"1234", "%+8.5d", "  +01234"},
#	{"1234", "%+8.6d", " +001234"},
#	{"-1234", "%+8.3d", "   -1234"},
#	{"-1234", "%+8.4d", "   -1234"},
#	{"-1234", "%+8.5d", "  -01234"},
#	{"-1234", "%+8.6d", " -001234"},
#
#	{"1234", "% 8.3d", "    1234"},
#	{"1234", "% 8.4d", "    1234"},
#	{"1234", "% 8.5d", "   01234"},
#	{"1234", "% 8.6d", "  001234"},
#	{"-1234", "% 8.3d", "   -1234"},
#	{"-1234", "% 8.4d", "   -1234"},
#	{"-1234", "% 8.5d", "  -01234"},
#	{"-1234", "% 8.6d", " -001234"},
#
#	{"1234", "%.3x", "4d2"},
#	{"1234", "%.4x", "04d2"},
#	{"1234", "%.5x", "004d2"},
#	{"1234", "%.6x", "0004d2"},
#	{"-1234", "%.3x", "-4d2"},
#	{"-1234", "%.4x", "-04d2"},
#	{"-1234", "%.5x", "-004d2"},
#	{"-1234", "%.6x", "-0004d2"},
#
#	{"1234", "%8.3x", "     4d2"},
#	{"1234", "%8.4x", "    04d2"},
#	{"1234", "%8.5x", "   004d2"},
#	{"1234", "%8.6x", "  0004d2"},
#	{"-1234", "%8.3x", "    -4d2"},
#	{"-1234", "%8.4x", "   -04d2"},
#	{"-1234", "%8.5x", "  -004d2"},
#	{"-1234", "%8.6x", " -0004d2"},
#
#	{"1234", "%+8.3x", "    +4d2"},
#	{"1234", "%+8.4x", "   +04d2"},
#	{"1234", "%+8.5x", "  +004d2"},
#	{"1234", "%+8.6x", " +0004d2"},
#	{"-1234", "%+8.3x", "    -4d2"},
#	{"-1234", "%+8.4x", "   -04d2"},
#	{"-1234", "%+8.5x", "  -004d2"},
#	{"-1234", "%+8.6x", " -0004d2"},
#
#	{"1234", "% 8.3x", "     4d2"},
#	{"1234", "% 8.4x", "    04d2"},
#	{"1234", "% 8.5x", "   004d2"},
#	{"1234", "% 8.6x", "  0004d2"},
#	{"1234", "% 8.7x", " 00004d2"},
#	{"1234", "% 8.8x", " 000004d2"},
#	{"-1234", "% 8.3x", "    -4d2"},
#	{"-1234", "% 8.4x", "   -04d2"},
#	{"-1234", "% 8.5x", "  -004d2"},
#	{"-1234", "% 8.6x", " -0004d2"},
#	{"-1234", "% 8.7x", "-00004d2"},
#	{"-1234", "% 8.8x", "-000004d2"},
#
#	{"1234", "%-8.3d", "1234    "},
#	{"1234", "%-8.4d", "1234    "},
#	{"1234", "%-8.5d", "01234   "},
#	{"1234", "%-8.6d", "001234  "},
#	{"1234", "%-8.7d", "0001234 "},
#	{"1234", "%-8.8d", "00001234"},
#	{"-1234", "%-8.3d", "-1234   "},
#	{"-1234", "%-8.4d", "-1234   "},
#	{"-1234", "%-8.5d", "-01234  "},
#	{"-1234", "%-8.6d", "-001234 "},
#	{"-1234", "%-8.7d", "-0001234"},
#	{"-1234", "%-8.8d", "-00001234"},
#
#	{"16777215", "%b", "111111111111111111111111"}, // 2**24 - 1
#
#	{"0", "%.d", ""},
#	{"0", "%.0d", ""},
#	{"0", "%3.d", ""},
]

func TestFormat(t: TestingT) -> void:
	testFormat(t)

static func sprintf(format: String, x: BigInt) -> String:
	## PORT: since Godot doesn't have interfaces like fmt.Formatter, let's just fake it for this test
	## (ported from (*big.Int).Format)
	assert(format[0] == '%')
	format = format.substr(1)

	var sharp := false
	var zero := false
	var plus := false
	var minus := false
	var space := false
	var width := 0
	var widthSet := false
	var precision := 0
	var precisionSet := false
	var ch := format[-1]

	var widthAndPrecision := format.substr(0, len(format) - 1).lstrip("#0+- ")
	for i in len(format) - 1 - len(widthAndPrecision):
		match format[i]:
			'#':
				sharp = true
			'0':
				zero = true
			'+':
				plus = true
			'-':
				minus = true
			' ':
				space = true
			_:
				assert(false)

	var wp := widthAndPrecision.split(".")
	widthSet = not wp[0].is_empty()
	width = wp[0].to_int()
	if len(wp) == 2:
		precisionSet = true
		precision = wp[1].to_int()

	## Format implements [fmt.Formatter]. It accepts the formats
	## 'b' (binary), 'o' (octal with 0 prefix), 'O' (octal with 0o prefix),
	## 'd' (decimal), 'x' (lowercase hexadecimal), and
	## 'X' (uppercase hexadecimal).
	## Also supported are the full suite of package fmt's format
	## flags for integral types, including '+' and ' ' for sign
	## control, '#' for leading zero in octal and for hexadecimal,
	## a leading "0x" or "0X" for "%#x" and "%#X" respectively,
	## specification of minimum digits precision, output field
	## width, space or zero padding, and '-' for left or right
	## justification.

	## determine base
	var base := 0
	match ch:
		'b':
			base = 2
		'o', 'O':
			base = 8
		'd', 's', 'v':
			base = 10
		'x', 'X':
			base = 16
		_:
			## unknown format
			return "%!" + ch + "(big.Int=" + sprintf("%v", x) + ")"

	if x == null:
		return "<nil>"

	## determine sign character
	var sign := ""
	if x._neg:
		sign = "-"
	elif plus: ## supersedes ' ' when both specified
		sign = "+"
	elif space:
		sign = " "

	## determine prefix characters for indicating output base
	var prefix := ""
	if sharp:
		match format[-1]:
			'b': ## binary
				prefix = "0b"
			'o': ## octal
				prefix = "0"
			'x': ## hexadecimal
				prefix = "0x"
			'X':
				prefix = "0X"
	if ch == 'O':
		prefix = "0o"

	var abs := BigInt.new()
	abs.Abs(x)
	var digits := abs.String(base)
	if ch == 'X':
		digits = digits.to_upper()

	## number of characters for the three classes of number padding
	var left := 0  ## space characters to left of digits for right justification ("%8d")
	var zeros := 0 ## zero characters (actually cs[0]) as left-most digits ("%.8d")
	var right := 0 ## space characters to right of digits for left justification ("%-8d")

	## determine number padding from precision: the least number of digits to output
	if precisionSet:
		if len(digits) < precision:
			zeros = precision - len(digits) ## count of zero padding
		elif digits == "0" and precision == 0:
			return "" ## print nothing if zero value (x == 0) and zero precision ("." or ".0")

	## determine field pad from width: the least number of characters to output
	var length := len(sign) + len(prefix) + zeros + len(digits)
	if widthSet and length < width: ## pad as specified
		var d := width - length
		if minus:
			## pad on the right with spaces; supersedes '0' when both specified
			right = d
		elif zero and not precisionSet:
			## pad with zeros unless precision also specified
			zeros = d
		else:
			## pad on the left with spaces
			left = d

	## print number as [left pad][sign][prefix][zero pad][digits][right pad]
	return " ".repeat(left) + sign + prefix + "0".repeat(zeros) + digits + " ".repeat(right)

static func testFormat(t: TestingT) -> void:
	for i in len(formatTests):
		var test := formatTests[i]
		var x: BigInt
		if test[0] != "<nil>":
			x = BigInt.new()
			var err := x.SetString(test[0], 0)
			if err != OK:
				t.Error("#%d failed reading input %s" % [i, test[0]])
		var output := sprintf(test[1], x)
		if output != test[2]:
			t.Error("#%d got \"%s\"; want \"%s\", {\"%s\", \"%s\", \"%s\"}" % [i, output, test[2], test[0], test[1], test[2]])

func BenchmarkFormat(b: TestingB) -> void:
	## This benchmark is very rough but it's good enough to show
	## https://go.dev/issue/71465 results.
	for i in b.N:
		testFormat(null)

#type scanTest struct {
#	input     string
#	format    string
#	output    string
#	remaining int
#}
#
#var scanTests = []scanTest{
#	{"1010", "%b", "10", 0},
#	{"0b1010", "%v", "10", 0},
#	{"12", "%o", "10", 0},
#	{"012", "%v", "10", 0},
#	{"10", "%d", "10", 0},
#	{"10", "%v", "10", 0},
#	{"a", "%x", "10", 0},
#	{"0xa", "%v", "10", 0},
#	{"A", "%X", "10", 0},
#	{"-A", "%X", "-10", 0},
#	{"+0b1011001", "%v", "89", 0},
#	{"0xA", "%v", "10", 0},
#	{"0 ", "%v", "0", 1},
#	{"2+3", "%v", "2", 2},
#	{"0XABC 12", "%v", "2748", 3},
#
#	{"10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000022222223333333333444444444", "%x", "72999049881955123498258745691204661198291656115976958889267080286388402675338838184094604981077942396458276955120179409196748346461468914795561487752253275293347599221664790586512596660792869956", 0},
#	{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff33377fffffffffffffffffffffffffffffffffffffffffffff0000000000022222eee1", "%x", "1167984798111281975972139931059274579172666497855631342228273284582214442805421410945513679697247078343332431249286160621687557589604464869034163736183926240549918956767671325412748661204059352801", 0},
#	{"5c0d52f451aec609b15da8e5e5626c4eaa88723bdeac9d25ca9b961269400410ca208a16af9c2fb07d7a11c7772cba02c22f9711078d51a3797eb18e691295293284d988e349fa6deba46b25a4ecd9f715", "%x", "419981998319789881681348172155240145539175961318447822049735313481433836043208347786919222066492311384432264836938599791362288343314139526391998172436831830624710446410781662672086936222288181013", 0},
#	{"92fcad4b5c0d52f451aec609b15da8e5e5626c4eaa88723bdeac9d25ca9b961269400410ca208a16af9c2fb07d799c32fe2f3cc5422f9711078d51a3797eb18e691295293284d8f5e69caf6decddfe1df6", "%x", "670619546945481998414061201992255225716434798957375727890607516800039934374391281275121813279544891602026798031004764406015624866771554937391445093144221697436880587924204655403711377861305572854", 0},
#	{"10000000000000000000000200000000000000000000003000000000000000000000040000000000000000000000500000000000000000000006", "%d", "10000000000000000000000200000000000000000000003000000000000000000000040000000000000000000000500000000000000000000006", 0},
#}
#
#func init() {
#	for i := range 200 {
#		d := make([]byte, i+1)
#		for j := range d {
#			d[j] = '0' + rand.N(byte(10))
#		}
#		if d[0] == '0' {
#			d[0] = '1'
#		}
#		scanTests = append(scanTests, scanTest{input: string(d), format: "%d", output: string(d)})
#	}
#}

func TestScan(t: TestingT) -> void:
	t.Error("TODO")
#	var buf bytes.Buffer
#	for i, test := range scanTests {
#		x := new(Int)
#		buf.Reset()
#		buf.WriteString(test.input)
#		if _, err := fmt.Fscanf(&buf, test.format, x); err != nil {
#			t.Errorf("#%d error: %s", i, err)
#		}
#		if x.String() != test.output {
#			t.Errorf("#%d got %s; want %s", i, x.String(), test.output)
#		}
#		if buf.Len() != test.remaining {
#			t.Errorf("#%d got %d bytes remaining; want %d", i, buf.Len(), test.remaining)
#		}
#	}
#}
