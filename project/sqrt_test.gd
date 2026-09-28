# Copyright 2017 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name SqrtTest
extends TestSuite

# TestFloatSqrt64 tests that Float.Sqrt of numbers with 53bit mantissa
# behaves like float math.Sqrt.
func TestFloatSqrt64(t: TestingT) -> void:
	var rand := RandomNumberGenerator.new()
	for i in 100000:
		var r := rand.randf()

		var got := BigFloat.new()
		got.SetPrec(53)
		got.Sqrt(BigFloat.NewFloat(r))
		var want := BigFloat.NewFloat(sqrt(r))
		if got.Cmp(want) != 0:
			t.Error("Sqrt(%f) =\n got %s;\nwant %s" % [r, got, want])
			return

func TestFloatSqrt(t: TestingT) -> void:
	for test in [
		# Test values were generated on Wolfram Alpha using query
		#   'sqrt(N) to 350 digits'
		# 350 decimal digits give up to 1000 binary digits.
		["0.03125", "0.17677669529663688110021109052621225982120898442211850914708496724884155980776337985629844179095519659187673077886403712811560450698134215158051518713749197892665283324093819909447499381264409775757143376369499645074628431682460775184106467733011114982619404115381053858929018135497032545349940642599871090667456829147610370507757690729404938184321879"],
		["0.125", "0.35355339059327376220042218105242451964241796884423701829416993449768311961552675971259688358191039318375346155772807425623120901396268430316103037427498395785330566648187639818894998762528819551514286752738999290149256863364921550368212935466022229965238808230762107717858036270994065090699881285199742181334913658295220741015515381458809876368643757"],
		["0.5", "0.70710678118654752440084436210484903928483593768847403658833986899536623923105351942519376716382078636750692311545614851246241802792536860632206074854996791570661133296375279637789997525057639103028573505477998580298513726729843100736425870932044459930477616461524215435716072541988130181399762570399484362669827316590441482031030762917619752737287514"],
		["2.0", "1.4142135623730950488016887242096980785696718753769480731766797379907324784621070388503875343276415727350138462309122970249248360558507372126441214970999358314132226659275055927557999505011527820605714701095599716059702745345968620147285174186408891986095523292304843087143214508397626036279952514079896872533965463318088296406206152583523950547457503"],
		["3.0", "1.7320508075688772935274463415058723669428052538103806280558069794519330169088000370811461867572485756756261414154067030299699450949989524788116555120943736485280932319023055820679748201010846749232650153123432669033228866506722546689218379712270471316603678615880190499865373798593894676503475065760507566183481296061009476021871903250831458295239598"],
		["4.0", "2.0"],

		["1p512", "1p256"],
		["4p1024", "2p512"],
		["9p2048", "3p1024"],

		["1p-1024", "1p-512"],
		["4p-2048", "2p-1024"],
		["9p-4096", "3p-2048"],
	]:
		for prec in [24, 53, 64, 65, 100, 128, 129, 200, 256, 400, 600, 800, 1000]:
			var x := BigFloat.new()
			x.SetPrec(prec)
			x.SetString(test[0], 10)

			var got := BigFloat.new()
			got.SetPrec(prec)
			got.Sqrt(x)

			var want := BigFloat.new()
			want.SetPrec(prec)
			want.SetString(test[1])

			if got.Cmp(want) != 0:
				t.Error("prec = %d, Sqrt(%s) =\ngot  %s;\nwant %s" % [prec, test[0], got, want])

			# Square test.
			# If got holds the square root of x to precision p, then
			#   got = √x + k
			# for some k such that |k| < 2**(-p). Thus,
			#   got² = (√x + k)² = x + 2k√n + k²
			# and the error must satisfy
			#   err = |got² - x| ≈ | 2k√n | < 2**(-p+1)*√n
			# Ignoring the k² term for simplicity.

			# err = |got² - x|
			# (but do intermediate steps with 32 guard digits to
			# avoid introducing spurious rounding-related errors)
			var sq := BigFloat.new()
			sq.SetPrec(prec + 32)
			sq.Mul(got, got)

			var err := BigFloat.new()
			err.Sub(sq, x)
			err.Abs(err)
			err.SetPrec(prec)

			# maxErr = 2**(-p+1)*√x
			var one := BigFloat.new()
			one.SetPrec(prec)
			one.SetInt64(1)

			var maxErr := BigFloat.new()
			maxErr.SetMantExp(one, 1 - prec)
			maxErr.Mul(maxErr, got)

			if err.Cmp(maxErr) >= 0:
				t.Error("prec = %d, Sqrt(%s) =\ngot err  %s;\nwant maxErr %s" % [prec, test[0], err, maxErr])

func TestFloatSqrtSpecial(t: TestingT) -> void:
	for test in [
		[BigFloat.NewFloat(+0.0), BigFloat.NewFloat(+0.0)],
		[BigFloat.NewFloat(-0.0), BigFloat.NewFloat(-0.0)],
		[BigFloat.NewFloat(INF), BigFloat.NewFloat(INF)],
	]:
		var got := BigFloat.new()
		got.Sqrt(test[0])
		var want: BigFloat = test[1]
		if got._neg != want._neg or got._form != want._form:
			t.Error("Sqrt(%s) = %s (neg: %s); want %s (neg: %s)" % [test[0], got, got._neg, want, want._neg])

# Benchmarks

func BenchmarkFloatSqrt(b0: TestingB) -> void:
	for prec in [64, 128, 256, 1000, 10000, 100000, 1000000]:
		var x := BigFloat.NewFloat(2)
		var z := BigFloat.new()
		z.SetPrec(prec)
		b0.Run("%d" % [prec], func(b: TestingB) -> void:
			for i in b.N:
				z.Sqrt(x))
