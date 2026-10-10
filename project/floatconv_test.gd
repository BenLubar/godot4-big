## This file is ported from src/math/big/floatconv_test.go in Go 1.27.1.
## Original copyright notice follows:

## Copyright 2015 The Go Authors. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

class_name FloatConvTest
extends TestSuite

func TestFloatSetFloat64String(t: TestingT) -> void:
	#struct {
	#	s string
	#	x float64 // NaNs represent invalid inputs
	#}
	var tests: Array[Array] = [
		## basics
		["0", FloatTest.float64_constant(0x0)],
		["-0", FloatTest.float64_constant(-0x8000000000000000)],
		["+0", FloatTest.float64_constant(0x0)],
		["1", FloatTest.float64_constant(0x3ff0000000000000)],
		["-1", FloatTest.float64_constant(-0x4010000000000000)],
		["+1", FloatTest.float64_constant(0x3ff0000000000000)],
		["1.234", FloatTest.float64_constant(0x3ff3be76c8b43958)],
		["-1.234", FloatTest.float64_constant(-0x400c4189374bc6a8)],
		["+1.234", FloatTest.float64_constant(0x3ff3be76c8b43958)],
		[".1", FloatTest.float64_constant(0x3fb999999999999a)],
		["1.", FloatTest.float64_constant(0x3ff0000000000000)],
		["+1.", FloatTest.float64_constant(0x3ff0000000000000)],

		## various zeros
		["0e100", FloatTest.float64_constant(0x0)],
		["-0e+100", FloatTest.float64_constant(-0x8000000000000000)],
		["+0e-100", FloatTest.float64_constant(0x0)],
		["0E100", FloatTest.float64_constant(0x0)],
		["-0E+100", FloatTest.float64_constant(-0x8000000000000000)],
		["+0E-100", FloatTest.float64_constant(0x0)],

		## various decimal exponent formats
		["1.e10", FloatTest.float64_constant(0x4202a05f20000000)],
		["1e+10", FloatTest.float64_constant(0x4202a05f20000000)],
		["+1e-10", FloatTest.float64_constant(0x3ddb7cdfd9d7bdbb)],
		["1E10", FloatTest.float64_constant(0x4202a05f20000000)],
		["1.E+10", FloatTest.float64_constant(0x4202a05f20000000)],
		["+1E-10", FloatTest.float64_constant(0x3ddb7cdfd9d7bdbb)],

		## infinities
		["Inf", INF],
		["+Inf", INF],
		["-Inf", -INF],
		["inf", INF],
		["+inf", INF],
		["-inf", -INF],

		## invalid numbers
		["", NAN],
		["-", NAN],
		["0x", NAN],
		["0e", NAN],
		["1.2ef", NAN],
		["2..3", NAN],
		["123..", NAN],
		["infinity", NAN],
		["foobar", NAN],

		## invalid underscores
		["_", NAN],
		["0_", NAN],
		["1__0", NAN],
		["123_.", NAN],
		["123._", NAN],
		["123._4", NAN],
		["1_2.3_4_", NAN],
		["_.123", NAN],
		["_123.456", NAN],
		["10._0", NAN],
		["10.0e_0", NAN],
		["10.0e0_", NAN],
		["0P-0__0", NAN],

		## misc decimal values
		["3.14159265", FloatTest.float64_constant(0x400921fb53c8d4f1)],
		["-687436.79457e-245", FloatTest.float64_constant(-0x71b914afcf936d43)],
		["-687436.79457E245", FloatTest.float64_constant(-0xbfccc100e324cb6)],
		[".0000000000000000000000000000000000000001", FloatTest.float64_constant(0x37a16c262777579c)],
		["+10000000000000000000000000000000000000000e-0", FloatTest.float64_constant(0x483d6329f1c35ca5)],

		## decimal mantissa, binary exponent
		["0p0", FloatTest.float64_constant(0x0)],
		["-0p0", FloatTest.float64_constant(-0x8000000000000000)],
		["1p10", FloatTest.float64_constant(0x4090000000000000)],
		["1p+10", FloatTest.float64_constant(0x4090000000000000)],
		["+1p-10", FloatTest.float64_constant(0x3f50000000000000)],
		["1024p-12", FloatTest.float64_constant(0x3fd0000000000000)],
		["-1p10", FloatTest.float64_constant(-0x3f70000000000000)],
		["1.5p1", FloatTest.float64_constant(0x4008000000000000)],

		## binary mantissa, decimal exponent
		["0b0", FloatTest.float64_constant(0x0)],
		["-0b0", FloatTest.float64_constant(-0x8000000000000000)],
		["0b0e+10", FloatTest.float64_constant(0x0)],
		["-0b0e-10", FloatTest.float64_constant(-0x8000000000000000)],
		["0b1010", FloatTest.float64_constant(0x4024000000000000)],
		["0B1010E2", FloatTest.float64_constant(0x408f400000000000)],
		["0b.1", FloatTest.float64_constant(0x3fe0000000000000)],
		["0b.001", FloatTest.float64_constant(0x3fc0000000000000)],
		["0b.001e3", FloatTest.float64_constant(0x405f400000000000)],

		## binary mantissa, binary exponent
		["0b0p+10", FloatTest.float64_constant(0x0)],
		["-0b0p-10", FloatTest.float64_constant(-0x8000000000000000)],
		["0b.1010p4", FloatTest.float64_constant(0x4024000000000000)],
		["0b1p-1", FloatTest.float64_constant(0x3fe0000000000000)],
		["0b001p-3", FloatTest.float64_constant(0x3fc0000000000000)],
		["0b.001p3", FloatTest.float64_constant(0x3ff0000000000000)],
		["0b0.01p2", FloatTest.float64_constant(0x3ff0000000000000)],
		["0b0.01P+2", FloatTest.float64_constant(0x3ff0000000000000)],

		## octal mantissa, decimal exponent
		["0o0", FloatTest.float64_constant(0x0)],
		["-0o0", FloatTest.float64_constant(-0x8000000000000000)],
		["0o0e+10", FloatTest.float64_constant(0x0)],
		["-0o0e-10", FloatTest.float64_constant(-0x8000000000000000)],
		["0o12", FloatTest.float64_constant(0x4024000000000000)],
		["0O12E2", FloatTest.float64_constant(0x408f400000000000)],
		["0o.4", FloatTest.float64_constant(0x3fe0000000000000)],
		["0o.01", FloatTest.float64_constant(0x3f90000000000000)],
		["0o.01e3", FloatTest.float64_constant(0x402f400000000000)],

		## octal mantissa, binary exponent
		["0o0p+10", FloatTest.float64_constant(0x0)],
		["-0o0p-10", FloatTest.float64_constant(-0x8000000000000000)],
		["0o.12p6", FloatTest.float64_constant(0x4024000000000000)],
		["0o4p-3", FloatTest.float64_constant(0x3fe0000000000000)],
		["0o0014p-6", FloatTest.float64_constant(0x3fc8000000000000)],
		["0o.001p9", FloatTest.float64_constant(0x3ff0000000000000)],
		["0o0.01p7", FloatTest.float64_constant(0x4000000000000000)],
		["0O0.01P+2", FloatTest.float64_constant(0x3fb0000000000000)],

		## hexadecimal mantissa and exponent
		["0x0", FloatTest.float64_constant(0x0)],
		["-0x0", FloatTest.float64_constant(-0x8000000000000000)],
		["0x0p+10", FloatTest.float64_constant(0x0)],
		["-0x0p-10", FloatTest.float64_constant(-0x8000000000000000)],
		["0xff", FloatTest.float64_constant(0x406fe00000000000)],
		["0X.8p1", FloatTest.float64_constant(0x3ff0000000000000)],
		["-0X0.00008p16", FloatTest.float64_constant(-0x4020000000000000)],
		["-0X0.00008P+16", FloatTest.float64_constant(-0x4020000000000000)],
		["0x0.0000000000001p-1022", FloatTest.float64_constant(0x1)],
		["0x1.fffffffffffffp1023", FloatTest.float64_constant(0x7fefffffffffffff)],

		## underscores
		["0_0", FloatTest.float64_constant(0x0)],
		["1_000.", FloatTest.float64_constant(0x408f400000000000)],
		["1_2_3.4_5_6", FloatTest.float64_constant(0x405edd2f1a9fbe77)],
		["1.0e0_0", FloatTest.float64_constant(0x3ff0000000000000)],
		["1p+1_0", FloatTest.float64_constant(0x4090000000000000)],
		["0b_1000", FloatTest.float64_constant(0x4020000000000000)],
		["0b_1011_1101", FloatTest.float64_constant(0x4067a00000000000)],
		["0x_f0_0d_1eP+0_8", FloatTest.float64_constant(0x41ee01a3c0000000)],
	]
	for test in tests:
		var x := BigFloat.new()
		x.SetPrec(53)
		var err := x.SetString(test[0])
		if is_nan(test[1]):
			## test.s is invalid
			if err == OK:
				t.Error("%s: want parse error" % [test[0]])
			continue

		## test.s is valid
		if err != OK:
			t.Error("%s: got parse error" % [test[0]])
			continue

		var f := x.Float64()
		var want := BigFloat.new()
		want.SetFloat64(test[1])
		if x.Cmp(want) != 0 or x.Signbit() != want.Signbit():
			t.Error("%s: got %s (%f); want %s" % [test[0], x, f, test[1]])

func TestFloat64Text(t: TestingT) -> void:
	t.Error("TODO")
	#struct {
	#	x      float64
	#	format byte
	#	prec   int
	#	want   string
	#}
	var tests: Array[Array] = [
		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_PLAIN, 0, "0"],
		[FloatTest.float64_constant(-0x8000000000000000), BigFloat.FORMAT_PLAIN, 0, "-0"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_PLAIN, 0, "1"],
		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_PLAIN, 0, "-1"],

		[FloatTest.float64_constant(0x3f50624dd2f1a9fc), BigFloat.FORMAT_SCIENTIFIC, 0, "1e-03"],
		[FloatTest.float64_constant(0x3fdd604189374bc7), BigFloat.FORMAT_SCIENTIFIC, 0, "5e-01"],
		[FloatTest.float64_constant(0x3ff75810624dd2f2), BigFloat.FORMAT_SCIENTIFIC, 0, "1e+00"],
		[FloatTest.float64_constant(0x4003ac083126e979), BigFloat.FORMAT_SCIENTIFIC, 1, "2.5e+00"],
		[FloatTest.float64_constant(0x400bac083126e979), BigFloat.FORMAT_SCIENTIFIC, 2, "3.46e+00"],
		[FloatTest.float64_constant(0x4011d604189374bc), BigFloat.FORMAT_SCIENTIFIC, 3, "4.459e+00"],
		[FloatTest.float64_constant(0x4015d604189374bc), BigFloat.FORMAT_SCIENTIFIC, 4, "5.4590e+00"],

		[FloatTest.float64_constant(0x3f50624dd2f1a9fc), BigFloat.FORMAT_PLAIN, 0, "0"],
		[FloatTest.float64_constant(0x3fdd604189374bc7), BigFloat.FORMAT_PLAIN, 0, "0"],
		[FloatTest.float64_constant(0x3ff75810624dd2f2), BigFloat.FORMAT_PLAIN, 0, "1"],
		[FloatTest.float64_constant(0x4003ac083126e979), BigFloat.FORMAT_PLAIN, 1, "2.5"],
		[FloatTest.float64_constant(0x400bac083126e979), BigFloat.FORMAT_PLAIN, 2, "3.46"],
		[FloatTest.float64_constant(0x4011d604189374bc), BigFloat.FORMAT_PLAIN, 3, "4.459"],
		[FloatTest.float64_constant(0x4015d604189374bc), BigFloat.FORMAT_PLAIN, 4, "5.4590"],

		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_DECIMAL, 0, "0"],
		[FloatTest.float64_constant(-0x8000000000000000), BigFloat.FORMAT_DECIMAL, 0, "-0"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_DECIMAL, 0, "4503599627370496p-52"],
		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_DECIMAL, 0, "-4503599627370496p-52"],
		[FloatTest.float64_constant(0x4330000000000000), BigFloat.FORMAT_DECIMAL, 0, "4503599627370496p+0"],

		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_HEX_NORMAL, 0, "0"],
		[FloatTest.float64_constant(-0x8000000000000000), BigFloat.FORMAT_HEX_NORMAL, 0, "-0"],
		[FloatTest.float64_constant(0x4090000000000000), BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p+11"],
		[FloatTest.float64_constant(-0x3f70000000000000), BigFloat.FORMAT_HEX_NORMAL, 0, "-0x.8p+11"],

		## all test cases below from strconv/ftoa_test.go
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_SCIENTIFIC, 5, "1.00000e+00"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_PLAIN, 5, "1.00000"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_AUTO, 5, "1"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_AUTO, -1, "1"],
		[FloatTest.float64_constant(0x4034000000000000), BigFloat.FORMAT_AUTO, -1, "20"],
		[FloatTest.float64_constant(0x4132d687cccccccd), BigFloat.FORMAT_AUTO, -1, "1.2345678e+06"],
		[FloatTest.float64_constant(0x41086a0000000000), BigFloat.FORMAT_AUTO, -1, "200000"],
		[FloatTest.float64_constant(0x413e848000000000), BigFloat.FORMAT_AUTO, -1, "2e+06"],

		## g conversion and zero suppression
		[FloatTest.float64_constant(0x4079000000000000), BigFloat.FORMAT_AUTO, 2, "4e+02"],
		[FloatTest.float64_constant(0x4044000000000000), BigFloat.FORMAT_AUTO, 2, "40"],
		[FloatTest.float64_constant(0x4010000000000000), BigFloat.FORMAT_AUTO, 2, "4"],
		[FloatTest.float64_constant(0x3fd999999999999a), BigFloat.FORMAT_AUTO, 2, "0.4"],
		[FloatTest.float64_constant(0x3fa47ae147ae147b), BigFloat.FORMAT_AUTO, 2, "0.04"],
		[FloatTest.float64_constant(0x3f70624dd2f1a9fc), BigFloat.FORMAT_AUTO, 2, "0.004"],
		[FloatTest.float64_constant(0x3f3a36e2eb1c432d), BigFloat.FORMAT_AUTO, 2, "0.0004"],
		[FloatTest.float64_constant(0x3f04f8b588e368f1), BigFloat.FORMAT_AUTO, 2, "4e-05"],
		[FloatTest.float64_constant(0x3ed0c6f7a0b5ed8d), BigFloat.FORMAT_AUTO, 2, "4e-06"],

		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_SCIENTIFIC, 5, "0.00000e+00"],
		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_PLAIN, 5, "0.00000"],
		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_AUTO, 5, "0"],
		[FloatTest.float64_constant(0x0), BigFloat.FORMAT_AUTO, -1, "0"],

		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_SCIENTIFIC, 5, "-1.00000e+00"],
		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_PLAIN, 5, "-1.00000"],
		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_AUTO, 5, "-1"],
		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_AUTO, -1, "-1"],

		[FloatTest.float64_constant(0x4028000000000000), BigFloat.FORMAT_SCIENTIFIC, 5, "1.20000e+01"],
		[FloatTest.float64_constant(0x4028000000000000), BigFloat.FORMAT_PLAIN, 5, "12.00000"],
		[FloatTest.float64_constant(0x4028000000000000), BigFloat.FORMAT_AUTO, 5, "12"],
		[FloatTest.float64_constant(0x4028000000000000), BigFloat.FORMAT_AUTO, -1, "12"],

		[FloatTest.float64_constant(0x419d6f32f0000000), BigFloat.FORMAT_SCIENTIFIC, 5, "1.23457e+08"],
		[FloatTest.float64_constant(0x419d6f32f0000000), BigFloat.FORMAT_PLAIN, 5, "123456700.00000"],
		[FloatTest.float64_constant(0x419d6f32f0000000), BigFloat.FORMAT_AUTO, 5, "1.2346e+08"],
		[FloatTest.float64_constant(0x419d6f32f0000000), BigFloat.FORMAT_AUTO, -1, "1.234567e+08"],

		[FloatTest.float64_constant(0x4132d64400000000), BigFloat.FORMAT_SCIENTIFIC, 5, "1.23450e+06"],
		[FloatTest.float64_constant(0x4132d64400000000), BigFloat.FORMAT_PLAIN, 5, "1234500.00000"],
		[FloatTest.float64_constant(0x4132d64400000000), BigFloat.FORMAT_AUTO, 5, "1.2345e+06"],

		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_SCIENTIFIC, 17, "9.99999999999999916e+22"],
		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_PLAIN, 17, "99999999999999991611392.00000000000000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_AUTO, 17, "9.9999999999999992e+22"],

		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_SCIENTIFIC, -1, "1e+23"],
		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_PLAIN, -1, "100000000000000000000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af6), BigFloat.FORMAT_AUTO, -1, "1e+23"],

		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_SCIENTIFIC, 17, "9.99999999999999748e+22"],
		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_PLAIN, 17, "99999999999999974834176.00000000000000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_AUTO, 17, "9.9999999999999975e+22"],

		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_SCIENTIFIC, -1, "9.999999999999997e+22"],
		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_PLAIN, -1, "99999999999999970000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af5), BigFloat.FORMAT_AUTO, -1, "9.999999999999997e+22"],

		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_SCIENTIFIC, 17, "1.00000000000000008e+23"],
		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_PLAIN, 17, "100000000000000008388608.00000000000000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_AUTO, 17, "1.0000000000000001e+23"],

		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_SCIENTIFIC, -1, "1.0000000000000001e+23"],
		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_PLAIN, -1, "100000000000000010000000"],
		[FloatTest.float64_constant(0x44b52d02c7e14af7), BigFloat.FORMAT_AUTO, -1, "1.0000000000000001e+23"],

		[FloatTest.float64_constant(0x1), BigFloat.FORMAT_AUTO, -1, "5e-324"],
		[FloatTest.float64_constant(-0x7fffffffffffffff), BigFloat.FORMAT_AUTO, -1, "-5e-324"],

		[FloatTest.float64_constant(0x4040000000000000), BigFloat.FORMAT_AUTO, -1, "32"],
		[FloatTest.float64_constant(0x4040000000000000), BigFloat.FORMAT_AUTO, 0, "3e+01"],

		[FloatTest.float64_constant(0x4059000000000000), BigFloat.FORMAT_HEX, -1, "0x1.9p+06"],

		#[FloatTest.float64_constant(0x7ff8000000000001), BigFloat.FORMAT_AUTO, -1, "NaN"],  ## Float doesn't support NaNs
		#[FloatTest.float64_constant(-0x7ffffffffffff), BigFloat.FORMAT_AUTO, -1, "NaN"], ## Float doesn't support NaNs
		[FloatTest.float64_constant(0x7ff0000000000000), BigFloat.FORMAT_AUTO, -1, "+Inf"],
		[FloatTest.float64_constant(-0x10000000000000), BigFloat.FORMAT_AUTO, -1, "-Inf"],

		[FloatTest.float64_constant(-0x4010000000000000), BigFloat.FORMAT_DECIMAL, -1, "-4503599627370496p-52"],

		## fixed bugs
		[FloatTest.float64_constant(0x3feccccccccccccd), BigFloat.FORMAT_PLAIN, 1, "0.9"],
		[FloatTest.float64_constant(0x3fb70a3d70a3d70a), BigFloat.FORMAT_PLAIN, 1, "0.1"],
		[FloatTest.float64_constant(0x3fb9930be0ded289), BigFloat.FORMAT_PLAIN, 1, "0.1"],
		[FloatTest.float64_constant(0x3fa999999999999a), BigFloat.FORMAT_PLAIN, 1, "0.1"],
		[FloatTest.float64_constant(0x3fa999999999999a), BigFloat.FORMAT_PLAIN, 0, "0"],
		[FloatTest.float64_constant(0x3fe0000000000000), BigFloat.FORMAT_PLAIN, 1, "0.5"],
		[FloatTest.float64_constant(0x3fe0000000000000), BigFloat.FORMAT_PLAIN, 0, "0"],
		[FloatTest.float64_constant(0x3ff8000000000000), BigFloat.FORMAT_PLAIN, 0, "2"],

		## https://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
		[FloatTest.float64_constant(0x10000000000000), BigFloat.FORMAT_AUTO, -1, "2.2250738585072014e-308"],
		## https://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
		[FloatTest.float64_constant(0xfffffffffffff), BigFloat.FORMAT_AUTO, -1, "2.225073858507201e-308"],

		## Issue 2625.
		[FloatTest.float64_constant(0x43954675da2b7e89), BigFloat.FORMAT_PLAIN, 0, "383260575764816448"],
		[FloatTest.float64_constant(0x43954675da2b7e89), BigFloat.FORMAT_AUTO, -1, "3.8326057576481645e+17"],

		## Issue 15918.
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_PLAIN, -10, "1"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_PLAIN, -11, "1"],
		[FloatTest.float64_constant(0x3ff0000000000000), BigFloat.FORMAT_PLAIN, -12, "1"],
	]
	for test in tests:
		## The test cases are from the strconv package which tests float64 values.
		## When formatting values with prec = -1 (shortest representation),
		## the actually available mantissa precision matters.
		## For denormalized values, that precision is < 53 (SetFloat64 default).
		## Compute and set the actual precision explicitly.
		var f := BigFloat.new()
		f.SetPrec(actualPrec(test[0]))
		f.SetFloat64(test[0])
		var got := f.String(test[1], test[2])
		if got != test[3]:
			t.Error("%s: got %s; want %s" % [test, got, test[3]])

## actualPrec returns the number of actually used mantissa bits.
func actualPrec(x: float) -> int:
	var buf: PackedByteArray
	buf.resize(8)
	buf.encode_double(0, x)
	var mant := buf.decode_u64(0)
	if x != 0 and (mant & (0x7ff << 52)) == 0:
		## x is denormalized
		mant &= ((1 << 52) - 1)
		var n := 0
		if mant >= (1 << 32):
			mant >>= 32
			n += 32
		if mant >= (1 << 16):
			mant >>= 16
			n += 16
		if mant >= (1 << 8):
			mant >>= 8
			n += 8
		if mant >= (1 << 4):
			mant >>= 4
			n += 4
		if mant >= (1 << 1):
			mant >>= 2
			n += 2
		if mant >= (1 << 2):
			mant >>= 2
			n += 2
		return n + 1
	return 53

func TestRoundShortestNormal(t: TestingT) -> void:
	## See go.dev/issue/80206.
	var text := func(x: float) -> String:
		var xf := BigFloat.new()
		xf.SetPrec(53)
		xf.SetFloat64(x)
		return xf.String(BigFloat.FORMAT_AUTO, -1)
	for test in [
		[FloatTest.float64_constant(0x4398493fba64eef3), "4.3749999999999917e+17"],
		[FloatTest.float64_constant(0x439bc16d674ec7f3), "4.9999999999999917e+17"],
		[FloatTest.float64_constant(0x439a6f12dc4b079b), "4.7619047619047597e+17"],
		[FloatTest.float64_constant(0x4394d1120d7b15f3), "3.7499999999999917e+17"],
		[FloatTest.float64_constant(0x43ba6f12dc4b079b), "1.9047619047619039e+18"],
		[FloatTest.float64_constant(0x43aeea4e1a08ad8f), "1.1138394197049199e+18"], ## m='9' at trimmed position; guard must reject round-up
	]:
		var got: String = text.call(test[0])
		var want: String = test[1]
		if got != want:
			t.Error("got %s, want %s" % [got, want])

func TestFloatText(t: TestingT) -> void:
	const defaultRound := -1 as BigFloat.RoundingMode

	#struct {
	#	x      string
	#	round  RoundingMode
	#	prec   uint
	#	format byte
	#	digits int
	#	want   string
	#}
	const tests: Array[Array] = [
		["0", defaultRound, 10, BigFloat.FORMAT_PLAIN, 0, "0"],
		["-0", defaultRound, 10, BigFloat.FORMAT_PLAIN, 0, "-0"],
		["1", defaultRound, 10, BigFloat.FORMAT_PLAIN, 0, "1"],
		["-1", defaultRound, 10, BigFloat.FORMAT_PLAIN, 0, "-1"],

		["1.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 0, "1e+00"],
		["2.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 1, "2.5e+00"],
		["3.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 2, "3.46e+00"],
		["4.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 3, "4.459e+00"],
		["5.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 4, "5.4590e+00"],

		["1.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC_UPPER, 0, "1E+00"],
		["2.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC_UPPER, 1, "2.5E+00"],
		["3.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC_UPPER, 2, "3.46E+00"],
		["4.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC_UPPER, 3, "4.459E+00"],
		["5.459", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC_UPPER, 4, "5.4590E+00"],

		["1.459", defaultRound, 100, BigFloat.FORMAT_PLAIN, 0, "1"],
		["2.459", defaultRound, 100, BigFloat.FORMAT_PLAIN, 1, "2.5"],
		["3.459", defaultRound, 100, BigFloat.FORMAT_PLAIN, 2, "3.46"],
		["4.459", defaultRound, 100, BigFloat.FORMAT_PLAIN, 3, "4.459"],
		["5.459", defaultRound, 100, BigFloat.FORMAT_PLAIN, 4, "5.4590"],

		["1.459", defaultRound, 100, BigFloat.FORMAT_AUTO, 0, "1"],
		["2.459", defaultRound, 100, BigFloat.FORMAT_AUTO, 1, "2"],
		["3.459", defaultRound, 100, BigFloat.FORMAT_AUTO, 2, "3.5"],
		["4.459", defaultRound, 100, BigFloat.FORMAT_AUTO, 3, "4.46"],
		["5.459", defaultRound, 100, BigFloat.FORMAT_AUTO, 4, "5.459"],

		["1459", defaultRound, 53, BigFloat.FORMAT_AUTO, 0, "1e+03"],
		["2459", defaultRound, 53, BigFloat.FORMAT_AUTO, 1, "2e+03"],
		["3459", defaultRound, 53, BigFloat.FORMAT_AUTO, 2, "3.5e+03"],
		["4459", defaultRound, 53, BigFloat.FORMAT_AUTO, 3, "4.46e+03"],
		["5459", defaultRound, 53, BigFloat.FORMAT_AUTO, 4, "5459"],

		["1459", defaultRound, 53, BigFloat.FORMAT_AUTO_UPPER, 0, "1E+03"],
		["2459", defaultRound, 53, BigFloat.FORMAT_AUTO_UPPER, 1, "2E+03"],
		["3459", defaultRound, 53, BigFloat.FORMAT_AUTO_UPPER, 2, "3.5E+03"],
		["4459", defaultRound, 53, BigFloat.FORMAT_AUTO_UPPER, 3, "4.46E+03"],
		["5459", defaultRound, 53, BigFloat.FORMAT_AUTO_UPPER, 4, "5459"],

		["3", defaultRound, 10, BigFloat.FORMAT_SCIENTIFIC, 40, "3.0000000000000000000000000000000000000000e+00"],
		["3", defaultRound, 10, BigFloat.FORMAT_PLAIN, 40, "3.0000000000000000000000000000000000000000"],
		["3", defaultRound, 10, BigFloat.FORMAT_AUTO, 40, "3"],

		["3e40", defaultRound, 100, BigFloat.FORMAT_SCIENTIFIC, 40, "3.0000000000000000000000000000000000000000e+40"],
		["3e40", defaultRound, 100, BigFloat.FORMAT_PLAIN, 4, "30000000000000000000000000000000000000000.0000"],
		["3e40", defaultRound, 100, BigFloat.FORMAT_AUTO, 40, "3e+40"],

		## make sure "stupid" exponents don't stall the machine
		["1e1000000", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.88b3a28a05eade3ap+3321929"],
		["1e646456992", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.e883a0c5c8c7c42ap+2147483644"],
		["1e646456993", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "+Inf"],
		["1e1000000000", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "+Inf"],
		["1e-1000000", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.efb4542cc8ca418ap-3321928"],
		["1e-646456993", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.e17c8956983d9d59p-2147483647"],
		["1e-646456994", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0"],
		["1e-1000000000", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0"],

		## minimum and maximum values
		["1p2147483646", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p+2147483647"],
		["0x.8p2147483647", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p+2147483647"],
		["0x.8p-2147483647", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p-2147483647"],
		["1p-2147483649", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p-2147483648"],

		## TODO(gri) need tests for actual large Floats

		["0", defaultRound, 53, BigFloat.FORMAT_DECIMAL, 0, "0"],
		["-0", defaultRound, 53, BigFloat.FORMAT_DECIMAL, 0, "-0"],
		["1.0", defaultRound, 53, BigFloat.FORMAT_DECIMAL, 0, "4503599627370496p-52"],
		["-1.0", defaultRound, 53, BigFloat.FORMAT_DECIMAL, 0, "-4503599627370496p-52"],
		["4503599627370496", defaultRound, 53, BigFloat.FORMAT_DECIMAL, 0, "4503599627370496p+0"],

		## issue 9939
		["3", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],
		["03", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],
		["3.", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],
		["3.0", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],
		["3.00", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],
		["3.000", defaultRound, 350, BigFloat.FORMAT_DECIMAL, 0, "1720123961992553633708115671476565205597423741876210842803191629540192157066363606052513914832594264915968p-348"],

		["3", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],
		["03", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],
		["3.", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],
		["3.0", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],
		["3.00", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],
		["3.000", defaultRound, 350, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.cp+2"],

		["0", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0"],
		["-0", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "-0"],
		["1024.0", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "0x.8p+11"],
		["-1024.0", defaultRound, 64, BigFloat.FORMAT_HEX_NORMAL, 0, "-0x.8p+11"],

		["0", defaultRound, 64, BigFloat.FORMAT_HEX, -1, "0x0p+00"],
		["0", defaultRound, 64, BigFloat.FORMAT_HEX, 0, "0x0p+00"],
		["0", defaultRound, 64, BigFloat.FORMAT_HEX, 1, "0x0.0p+00"],
		["0", defaultRound, 64, BigFloat.FORMAT_HEX, 5, "0x0.00000p+00"],
		["3.25", defaultRound, 64, BigFloat.FORMAT_HEX, 0, "0x1p+02"],
		["-3.25", defaultRound, 64, BigFloat.FORMAT_HEX, 0, "-0x1p+02"],
		["3.25", defaultRound, 64, BigFloat.FORMAT_HEX, 1, "0x1.ap+01"],
		["-3.25", defaultRound, 64, BigFloat.FORMAT_HEX, 1, "-0x1.ap+01"],
		["3.25", defaultRound, 64, BigFloat.FORMAT_HEX, -1, "0x1.ap+01"],
		["-3.25", defaultRound, 64, BigFloat.FORMAT_HEX, -1, "-0x1.ap+01"],
		["1024.0", defaultRound, 64, BigFloat.FORMAT_HEX, 0, "0x1p+10"],
		["-1024.0", defaultRound, 64, BigFloat.FORMAT_HEX, 0, "-0x1p+10"],
		["1024.0", defaultRound, 64, BigFloat.FORMAT_HEX, 5, "0x1.00000p+10"],
		["8191.0", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fffp+12"],
		["8191.5", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fff8p+12"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fff88p+12"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff8p+12"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 3, "0x1.000p+13"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 0, "0x1p+13"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fff888p+12"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff9p+12"],

		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fff88p+12"],
		["8191.53125", BigFloat.TO_NEAREST_EVEN, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.53125", BigFloat.TO_NEAREST_AWAY, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.53125", BigFloat.TO_ZERO, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.53125", BigFloat.AWAY_FROM_ZERO, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.53125", BigFloat.TO_NEGATIVE_INF, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.53125", BigFloat.TO_POSITIVE_INF, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],

		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff8p+12"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 3, "0x1.000p+13"],
		["8191.53125", defaultRound, 53, BigFloat.FORMAT_HEX, 0, "0x1p+13"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, -1, "0x1.fff888p+12"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, 6, "0x1.fff888p+12"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, 5, "0x1.fff88p+12"],
		["8191.533203125", defaultRound, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff9p+12"],

		["8191.53125", BigFloat.TO_NEAREST_EVEN, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff8p+12"],
		["8191.53125", BigFloat.TO_NEAREST_AWAY, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff9p+12"],
		["8191.53125", BigFloat.TO_ZERO, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff8p+12"],
		["8191.53125", BigFloat.TO_ZERO, 53, BigFloat.FORMAT_HEX, 2, "0x1.ffp+12"],
		["8191.53125", BigFloat.AWAY_FROM_ZERO, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff9p+12"],
		["8191.53125", BigFloat.TO_NEGATIVE_INF, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff8p+12"],
		["-8191.53125", BigFloat.TO_NEGATIVE_INF, 53, BigFloat.FORMAT_HEX, 4, "-0x1.fff9p+12"],
		["8191.53125", BigFloat.TO_POSITIVE_INF, 53, BigFloat.FORMAT_HEX, 4, "0x1.fff9p+12"],
		["-8191.53125", BigFloat.TO_POSITIVE_INF, 53, BigFloat.FORMAT_HEX, 4, "-0x1.fff8p+12"],

		## issue 34343
		["0x.8p-2147483648", BigFloat.TO_NEAREST_EVEN, 4, BigFloat.FORMAT_HEX_NORMAL, -1, "0x.8p-2147483648"],
		["0x.8p-2147483648", BigFloat.TO_NEAREST_EVEN, 4, BigFloat.FORMAT_HEX, -1, "0x1p-2147483649"],
	]

	for test in tests:
		var f := BigFloat.new()
		f.SetPrec(test[2])
		f.SetMode(BigFloat.TO_NEAREST_EVEN)

		var err := f.SetString(test[0])
		if err != OK:
			t.Error("%s: %s" % [test, error_string(err)])
			continue

		if test[1] != defaultRound:
			f.SetMode(test[1])

		var got := f.String(test[3], test[4])
		if got != test[5]:
			t.Error("%s: got %s; want %s" % [test, got, test[5]])

func BenchmarkParseFloatSmallExp(b: TestingB) -> void:
	const tests: PackedStringArray = [
		"1e0",
		"1e-1",
		"1e-2",
		"1e-3",
		"1e-4",
		"1e-5",
		"1e-10",
		"1e-20",
		"1e-50",
		"1e1",
		"1e2",
		"1e3",
		"1e4",
		"1e5",
		"1e10",
		"1e20",
		"1e50",
	]
	for i in b.N:
		for s in tests:
			var x := BigFloat.new()
			var err := x.SetString(s)
			assert(err == OK)

func BenchmarkParseFloatLargeExp(b: TestingB) -> void:
	const tests: PackedStringArray = [
		"1e0",
		"1e-10",
		"1e-20",
		"1e-30",
		"1e-40",
		"1e-50",
		"1e-100",
		"1e-500",
		"1e-1000",
		"1e-5000",
		"1e-10000",
		"1e10",
		"1e20",
		"1e30",
		"1e40",
		"1e50",
		"1e100",
		"1e500",
		"1e1000",
		"1e5000",
		"1e10000",
	]
	for i in b.N:
		for s in tests:
			var x := BigFloat.new()
			var err := x.SetString(s)
			assert(err == OK)
