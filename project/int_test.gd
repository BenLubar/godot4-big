# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

class_name IntTest
extends TestSuite

static func isNormalized(x: BigInt) -> bool:
	if x._abs.is_empty():
		return not x._neg
	# len(x._abs) > 0
	return x._abs[len(x._abs) - 1] != 0

static var sumZZ := [
	[BigInt.NewInt(0), BigInt.NewInt(0), BigInt.NewInt(0)],
	[BigInt.NewInt(1), BigInt.NewInt(1), BigInt.NewInt(0)],
	[BigInt.NewInt(1111111110), BigInt.NewInt(123456789), BigInt.NewInt(987654321)],
	[BigInt.NewInt(-1), BigInt.NewInt(-1), BigInt.NewInt(0)],
	[BigInt.NewInt(864197532), BigInt.NewInt(-123456789), BigInt.NewInt(987654321)],
	[BigInt.NewInt(-1111111110), BigInt.NewInt(-123456789), BigInt.NewInt(-987654321)],
]

static var prodZZ := [
	[BigInt.NewInt(0), BigInt.NewInt(0), BigInt.NewInt(0)],
	[BigInt.NewInt(0), BigInt.NewInt(1), BigInt.NewInt(0)],
	[BigInt.NewInt(1), BigInt.NewInt(1), BigInt.NewInt(1)],
	[BigInt.NewInt(-991 * 991), BigInt.NewInt(991), BigInt.NewInt(-991)],
	# TODO(gri) add larger products
]

func TestSignZ(t: TestingT) -> void:
	var zero := BigInt.new()
	for a in sumZZ:
		var z: BigInt = a[0]
		var s := z.Sign()
		var e := z.Cmp(zero)
		if s != e:
			t.Error("got %d; want %d for z = %s" % [s, e, a[0]])

func TestSetZ(t: TestingT) -> void:
	for a in sumZZ:
		var z := BigInt.new()
		z.Set(a[0])
		if not isNormalized(z):
			t.Error("%s is not normalized" % [z])
		if z.Cmp(a[0]) != 0:
			t.Error("got z = %s; want %s" % [z, a[0]])

func TestAbsZ(t: TestingT) -> void:
	var zero := BigInt.new()
	for a in sumZZ:
		var z := BigInt.new()
		z.Abs(a[0])
		var e := BigInt.new()
		e.Set(a[0])
		if e.Cmp(zero) < 0:
			e.Sub(zero, e)
		if z.Cmp(e) != 0:
			t.Error("got z = %s; want %s" % [z, e])

static func testFunZZ(t: TestingT, msg: String, f: Callable, a: Array) -> void:
	var z := BigInt.new()
	f.call(z, a[1], a[2])
	if not isNormalized(z):
		t.Error("%s: %s is not normalized" % [msg, z])
	if z.Cmp(a[0]) != 0:
		t.Error("%s %s %s\n\tgot z = %s; want %s" % [a[1], msg, a[2], z, a[0]])

func TestSumZZ(t: TestingT) -> void:
	var AddZZ := func(z: BigInt, x: BigInt, y: BigInt) -> void:
		z.Add(x, y)
	var SubZZ := func(z: BigInt, x: BigInt, y: BigInt) -> void:
		z.Sub(x, y)
	for a in sumZZ:
		testFunZZ(t, "AddZZ", AddZZ, a)
		testFunZZ(t, "AddZZ symmetric", AddZZ, [a[0], a[2], a[1]])
		testFunZZ(t, "SubZZ", SubZZ, [a[1], a[0], a[2]])
		testFunZZ(t, "SubZZ symmetric", SubZZ, [a[2], a[0], a[1]])

func TestProdZZ(t: TestingT) -> void:
	var MulZZ := func(z: BigInt, x: BigInt, y: BigInt) -> void:
		z.Mul(x, y)
	for a in prodZZ:
		testFunZZ(t, "MulZZ", MulZZ, a)
		testFunZZ(t, "MulZZ symmetric", MulZZ, [a[0], a[2], a[1]])

# mulBytes returns x*y via grade school multiplication. Both inputs
# and the result are assumed to be in big-endian representation (to
# match the semantics of Int.Bytes and Int.SetBytes).
static func mulBytes(x: PackedByteArray, y: PackedByteArray) -> PackedByteArray:
	var z: PackedByteArray
	z.resize(len(x) + len(y))

	# multiply
	var k0 := len(z) - 1
	for j in range(len(y) - 1, -1, -1):
		var d := y[j]
		if d != 0:
			var k := k0
			var carry := 0
			for i in range(len(x) - 1, -1, -1):
				var t := z[k] + x[i] * d + carry
				z[k] = t
				carry = t >> 8
				k -= 1
			z[k] = carry
		k0 -= 1

	# normalize (remove leading 0's)
	var i := 0
	while i < len(z) and z[i] == 0:
		i += 1

	return z.slice(i)

static func checkMul(a: PackedByteArray, b: PackedByteArray) -> bool:
	var x := BigInt.new()
	var y := BigInt.new()
	var z1 := BigInt.new()

	x.SetBytes(a)
	y.SetBytes(b)
	z1.Mul(x, y)

	var z2 := BigInt.new()
	z2.SetBytes(mulBytes(a, b))

	return z1.Cmp(z2) == 0

func TestMul(t: TestingT) -> void:
	t.QuickCheckBB(checkMul)

const mulRangesZ: Array[Array] = [
	# entirely positive ranges are covered by mulRangesN
	[-1, 1, "0"],
	[-2, -1, "2"],
	[-3, -2, "6"],
	[-3, -1, "-6"],
	[1, 3, "6"],
	[-10, -10, "-10"],
	[0, -1, "1"],                      # empty range
	[-1, -100, "1"],                   # empty range
	[-1, 1, "0"],                      # range includes 0
	[-1000000000, 0, "0"],             # range includes 0
	[-1000000000, 1000000000, "0"],    # range includes 0
	[-10, -1, "3628800"],              # 10!
	[-20, -2, "-2432902008176640000"], # -20!
	[-99, -1, "-933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000"], # -99!

	# overflow situations
	[INT64_MAX - 0, INT64_MAX, "9223372036854775807"],
	[INT64_MAX - 1, INT64_MAX, "85070591730234615838173535747377725442"],
	[INT64_MAX - 2, INT64_MAX, "784637716923335094969050127519550606919189611815754530810"],
	[INT64_MAX - 3, INT64_MAX, "7237005577332262206126809393809643289012107973151163787181513908099760521240"],
]

func TestMulRangeZ(t: TestingT) -> void:
	var tmp := BigInt.new()
	# test entirely positive ranges
	for i in len(NatTest.mulRangesN):
		var r := NatTest.mulRangesN[i]
		# skip mulRangesN entries that overflow int64
		if r[0] < 0 or r[1] < 0:
			continue
		tmp.MulRange(r[0], r[1])
		var prod := tmp.String()
		if prod != r[2]:
			t.Error("#%da: got %s; want %s" % [i, prod, r[2]])
	# test other ranges
	for i in len(mulRangesZ):
		var r := mulRangesZ[i]
		tmp.MulRange(r[0], r[1])
		var prod := tmp.String()
		if prod != r[2]:
			t.Error("#%db: got %s; want %s" % [i, prod, r[2]])

func TestBinomial(t: TestingT) -> void:
	var z := BigInt.new()
	for test in [
		[0, 0, "1"],
		[0, 1, "0"],
		[1, 0, "1"],
		[1, 1, "1"],
		[1, 10, "0"],
		[4, 0, "1"],
		[4, 1, "4"],
		[4, 2, "6"],
		[4, 3, "4"],
		[4, 4, "1"],
		[10, 1, "10"],
		[10, 9, "10"],
		[10, 5, "252"],
		[11, 5, "462"],
		[11, 6, "462"],
		[100, 10, "17310309456440"],
		[100, 90, "17310309456440"],
		[1000, 10, "263409560461970212832400"],
		[1000, 990, "263409560461970212832400"],
	]:
		z.Binomial(test[0], test[1])
		var got := z.String()
		if got != test[2]:
			t.Error("Binomial(%d, %d) = %s; want %s" % [test[0], test[1], got, test[2]])

func BenchmarkBinomial(b: TestingB) -> void:
	var z := BigInt.new()
	for i in b.N:
		z.Binomial(1000, 990)

# Examples from the Go Language Spec, section "Arithmetic operators"
const divisionSignsTests: Array[Array] = [
	[5, 3, 1, 2, 1, 2],
	[-5, 3, -1, -2, -2, 1],
	[5, -3, -1, 2, -1, 2],
	[-5, -3, 1, -2, 2, 1],
	[1, 2, 0, 1, 0, 1],
	[8, 4, 2, 0, 2, 0],
]

func TestDivisionSigns(t: TestingT) -> void:
	for i in len(divisionSignsTests):
		var test := divisionSignsTests[i]
		var x := BigInt.NewInt(test[0])
		var y := BigInt.NewInt(test[1])
		var q := BigInt.NewInt(test[2])
		var r := BigInt.NewInt(test[3])
		var d := BigInt.NewInt(test[4])
		var m := BigInt.NewInt(test[5])

		var q1 := BigInt.new()
		var r1 := BigInt.new()
		q1.Quo(x, y)
		r1.Rem(x, y)
		if not isNormalized(q1):
			t.Error("#%d Quo: %s is not normalized" % [i, q1])
		if not isNormalized(r1):
			t.Error("#%d Rem: %s is not normalized" % [i, r1])
		if q1.Cmp(q) != 0 or r1.Cmp(r) != 0:
			t.Error("#%d QuoRem: got (%s, %s), want (%s, %s)" % [i, q1, r1, q, r])

		var q2 := BigInt.new()
		var r2 := BigInt.new()
		q2.QuoRem(x, y, r2)
		if not isNormalized(q2):
			t.Error("#%d Quo: %s is not normalized" % [i, q2])
		if not isNormalized(r2):
			t.Error("#%d Rem: %s is not normalized" % [i, r2])
		if q2.Cmp(q) != 0 or r2.Cmp(r) != 0:
			t.Error("#%d QuoRem: got (%s, %s), want (%s, %s)" % [i, q2, r2, q, r])

		var d1 := BigInt.new()
		var m1 := BigInt.new()
		d1.Div(x, y)
		m1.Mod(x, y)
		if not isNormalized(d1):
			t.Error("#%d Div: %s is not normalized" % [i, d1])
		if not isNormalized(m1):
			t.Error("#%d Mod: %s is not normalized" % [i, m1])
		if d1.Cmp(d) != 0 or m1.Cmp(m) != 0:
			t.Error("#%d DivMod: got (%s, %s), want (%s, %s)" % [i, d1, m1, d, m])

		var d2 := BigInt.new()
		var m2 := BigInt.new()
		d2.DivMod(x, y, m2)
		if not isNormalized(d2):
			t.Error("#%d Div: %s is not normalized" % [i, d2])
		if not isNormalized(m2):
			t.Error("#%d Mod: %s is not normalized" % [i, m2])
		if d2.Cmp(d) != 0 or m2.Cmp(m) != 0:
			t.Error("#%d DivMod: got (%s, %s), want (%s, %s)" % [i, d2, m2, d, m])

static func checkSetBytes(b: PackedByteArray) -> bool:
	var z := BigInt.new()
	z.SetBytes(b)

	var hex1 := z.Bytes().hex_encode()
	var hex2 := b.hex_encode()

	while len(hex1) < len(hex2):
		hex1 = "0" + hex1

	while len(hex1) > len(hex2):
		hex2 = "0" + hex2

	return hex1 == hex2

func TestSetBytes(t: TestingT) -> void:
	t.QuickCheckB(checkSetBytes)

static func checkBytes(b: PackedByteArray) -> bool:
	# trim leading zero bytes since Bytes() won't return them
	# (was issue 12231)
	while len(b) > 0 and b[0] == 0:
		b = b.slice(1)

	var z := BigInt.new()
	z.SetBytes(b)
	var b2 := z.Bytes()
	return b == b2

func TestBytes(t: TestingT) -> void:
	t.QuickCheckB(checkBytes)

static func checkQuo(x: PackedByteArray, y: PackedByteArray) -> bool:
	var u := BigInt.new()
	var v := BigInt.new()
	u.SetBytes(x)
	v.SetBytes(y)

	if len(v._abs) == 0:
		return true

	var q := BigInt.new()
	var r := BigInt.new()
	q.QuoRem(u, v, r)

	if r.Cmp(v) >= 0:
		return false

	var uprime := BigInt.new()
	uprime.Set(q)
	uprime.Mul(uprime, v)
	uprime.Add(uprime, r)

	return uprime.Cmp(u) == 0

const quoTests: Array[Array] = [
	[
		"476217953993950760840509444250624797097991362735329973741718102894495832294430498335824897858659711275234906400899559094370964723884706254265559534144986498357",
		"9353930466774385905609975137998169297361893554149986716853295022578535724979483772383667534691121982974895531435241089241440253066816724367338287092081996",
		"50911",
		"1",
	],
	[
		"11510768301994997771168",
		"1328165573307167369775",
		"8",
		"885443715537658812968",
	],
]

func TestQuo(t: TestingT) -> void:
	t.QuickCheckBB(checkQuo)

	for i in len(quoTests):
		var test := quoTests[i]

		var x := BigInt.new()
		var y := BigInt.new()
		var expectedQ := BigInt.new()
		var expectedR := BigInt.new()

		x.SetString(test[0], 10)
		y.SetString(test[1], 10)
		expectedQ.SetString(test[2], 10)
		expectedR.SetString(test[3], 10)

		var q := BigInt.new()
		var r := BigInt.new()
		q.QuoRem(x, y, r)

		if q.Cmp(expectedQ) != 0 or r.Cmp(expectedR) != 0:
			t.Error("#%d got (%s, %s) want (%s, %s)" % [i, q, r, expectedQ, expectedR])

func TestQuoStepD6(t: TestingT) -> void:
	# See Knuth, Volume 2, section 4.3.1, exercise 21. This code exercises
	# a code path which only triggers 1 in 10^19 cases.

	var u := BigInt.new()
	var v := BigInt.new()
	u._abs = [0, 0, INT64_MIN + 1, INT64_MAX]
	v._abs = [5, INT64_MIN + 2, INT64_MIN]

	var q := BigInt.new()
	var r := BigInt.new()
	q.QuoRem(u, v, r)

	const expectedQ64 := "18446744073709551613"
	const expectedR64 := "3138550867693340382088035895064302439801311770021610913807"
	if q.String() != expectedQ64 or r.String() != expectedR64:
		t.Error("got (%s, %s) want (%s, %s)" % [q, r, expectedQ64, expectedR64])

func BenchmarkQuoRem(b: TestingB) -> void:
	var x := BigInt.new()
	var y := BigInt.new()
	var q := BigInt.new()
	var r := BigInt.new()
	x.SetString("153980389784927331788354528594524332344709972855165340650588877572729725338415474372475094155672066328274535240275856844648695200875763869073572078279316458648124537905600131008790701752441155668003033945258023841165089852359980273279085783159654751552359397986180318708491098942831252291841441726305535546071")
	y.SetString("7746362281539803897849273317883545285945243323447099728551653406505888775727297253384154743724750941556720663282745352402758568446486952008757638690735720782793164586481245379056001310087907017524411556680030339452580238411650898523599802732790857831596547515523593979861803187084910989428312522918414417263055355460715745539358014631136245887418412633787074173796862711588221766398229333338511838891484974940633857861775630560092874987828057333663969469797013996401149696897591265769095952887917296740109742927689053276850469671231961384715398038978492733178835452859452433234470997285516534065058887757272972533841547437247509415567206632827453524027585684464869520087576386907357207827931645864812453790560013100879070175244115566800303394525802384116508985235998027327908578315965475155235939798618031870849109894283125229184144172630553554607112725169432413343763989564437170644270643461665184965150423819594083121075825")

	b.ResetTimer()
	for i in b.N:
		q.QuoRem(y, x, r)

const bitLenTests: Array[Array] = [
	["-1", 1],
	["0", 0],
	["1", 1],
	["2", 2],
	["4", 3],
	["0xabc", 12],
	["0x8000", 16],
	["0x80000000", 32],
	["0x800000000000", 48],
	["0x8000000000000000", 64],
	["0x80000000000000000000", 80],
	["-0x4000000000000000000000", 87],
]

func TestBitLen(t: TestingT) -> void:
	for i in len(bitLenTests):
		var test := bitLenTests[i]
		var x := BigInt.new()
		var err := x.SetString(test[0])
		if err != OK:
			t.Error("#%d test input invalid: %s" % [i, test[0]])
			continue

		var n := x.BitLen()
		if n != test[1]:
			t.Error("#%d got %d want %d" % [i, n, test[1]])

const expTests: Array[Array] = [
	# y <= 0
	["0", "0", "", "1"],
	["1", "0", "", "1"],
	["-10", "0", "", "1"],
	["1234", "-1", "", "1"],
	["1234", "-1", "0", "1"],
	["17", "-100", "1234", "865"],
	["2", "-100", "1234", ""],

	# m == 1
	["0", "0", "1", "0"],
	["1", "0", "1", "0"],
	["-10", "0", "1", "0"],
	["1234", "-1", "1", "0"],

	# misc
	["5", "1", "3", "2"],
	["5", "-7", "", "1"],
	["-5", "-7", "", "1"],
	["5", "0", "", "1"],
	["-5", "0", "", "1"],
	["5", "1", "", "5"],
	["-5", "1", "", "-5"],
	["-5", "1", "7", "2"],
	["-2", "3", "2", "0"],
	["5", "2", "", "25"],
	["1", "65537", "2", "1"],
	["0x8000000000000000", "2", "", "0x40000000000000000000000000000000"],
	["0x8000000000000000", "2", "6719", "4944"],
	["0x8000000000000000", "3", "6719", "5447"],
	["0x8000000000000000", "1000", "6719", "1603"],
	["0x8000000000000000", "1000000", "6719", "3199"],
	["0x8000000000000000", "-1000000", "6719", "3663"], # 3663 = ModInverse(3199, 6719) Issue #25865

	["0xffffffffffffffffffffffffffffffff", "0x12345678123456781234567812345678123456789", "0x01112222333344445555666677778889", "0x36168FA1DB3AAE6C8CE647E137F97A"],

	[
		"2938462938472983472983659726349017249287491026512746239764525612965293865296239471239874193284792387498274256129746192347",
		"298472983472983471903246121093472394872319615612417471234712061",
		"29834729834729834729347290846729561262544958723956495615629569234729836259263598127342374289365912465901365498236492183464",
		"23537740700184054162508175125554701713153216681790245129157191391322321508055833908509185839069455749219131480588829346291",
	],
	# test case for issue 8822
	[
		"11001289118363089646017359372117963499250546375269047542777928006103246876688756735760905680604646624353196869572752623285140408755420374049317646428185270079555372763503115646054602867593662923894140940837479507194934267532831694565516466765025434902348314525627418515646588160955862839022051353653052947073136084780742729727874803457643848197499548297570026926927502505634297079527299004267769780768565695459945235586892627059178884998772989397505061206395455591503771677500931269477503508150175717121828518985901959919560700853226255420793148986854391552859459511723547532575574664944815966793196961286234040892865",
		"0xB08FFB20760FFED58FADA86DFEF71AD72AA0FA763219618FE022C197E54708BB1191C66470250FCE8879487507CEE41381CA4D932F81C2B3F1AB20B539D50DCD",
		"0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73",
		"21484252197776302499639938883777710321993113097987201050501182909581359357618579566746556372589385361683610524730509041328855066514963385522570894839035884713051640171474186548713546686476761306436434146475140156284389181808675016576845833340494848283681088886584219750554408060556769486628029028720727393293111678826356480455433909233520504112074401376133077150471237549474149190242010469539006449596611576612573955754349042329130631128234637924786466585703488460540228477440853493392086251021228087076124706778899179648655221663765993962724699135217212118535057766739392069738618682722216712319320435674779146070442",
	],
	[
		"-0x1BCE04427D8032319A89E5C4136456671AC620883F2C4139E57F91307C485AD2D6204F4F87A58262652DB5DBBAC72B0613E51B835E7153BEC6068F5C8D696B74DBD18FEC316AEF73985CF0475663208EB46B4F17DD9DA55367B03323E5491A70997B90C059FB34809E6EE55BCFBD5F2F52233BFE62E6AA9E4E26A1D4C2439883D14F2633D55D8AA66A1ACD5595E778AC3A280517F1157989E70C1A437B849F1877B779CC3CDDEDE2DAA6594A6C66D181A00A5F777EE60596D8773998F6E988DEAE4CCA60E4DDCF9590543C89F74F603259FCAD71660D30294FBBE6490300F78A9D63FA660DC9417B8B9DDA28BEB3977B621B988E23D4D954F322C3540541BC649ABD504C50FADFD9F0987D58A2BF689313A285E773FF02899A6EF887D1D4A0D2",
		"0xB08FFB20760FFED58FADA86DFEF71AD72AA0FA763219618FE022C197E54708BB1191C66470250FCE8879487507CEE41381CA4D932F81C2B3F1AB20B539D50DCD",
		"0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73",
		"21484252197776302499639938883777710321993113097987201050501182909581359357618579566746556372589385361683610524730509041328855066514963385522570894839035884713051640171474186548713546686476761306436434146475140156284389181808675016576845833340494848283681088886584219750554408060556769486628029028720727393293111678826356480455433909233520504112074401376133077150471237549474149190242010469539006449596611576612573955754349042329130631128234637924786466585703488460540228477440853493392086251021228087076124706778899179648655221663765993962724699135217212118535057766739392069738618682722216712319320435674779146070442",
	],

	# test cases for issue 13907
	["0xffffffff00000001", "0xffffffff00000001", "0xffffffff00000001", "0"],
	["0xffffffffffffffff00000001", "0xffffffffffffffff00000001", "0xffffffffffffffff00000001", "0"],
	["0xffffffffffffffffffffffff00000001", "0xffffffffffffffffffffffff00000001", "0xffffffffffffffffffffffff00000001", "0"],
	["0xffffffffffffffffffffffffffffffff00000001", "0xffffffffffffffffffffffffffffffff00000001", "0xffffffffffffffffffffffffffffffff00000001", "0"],

	[
		"2",
		"0xB08FFB20760FFED58FADA86DFEF71AD72AA0FA763219618FE022C197E54708BB1191C66470250FCE8879487507CEE41381CA4D932F81C2B3F1AB20B539D50DCD",
		"0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73", # odd
		"0x6AADD3E3E424D5B713FCAA8D8945B1E055166132038C57BBD2D51C833F0C5EA2007A2324CE514F8E8C2F008A2F36F44005A4039CB55830986F734C93DAF0EB4BAB54A6A8C7081864F44346E9BC6F0A3EB9F2C0146A00C6A05187D0C101E1F2D038CDB70CB5E9E05A2D188AB6CBB46286624D4415E7D4DBFAD3BCC6009D915C406EED38F468B940F41E6BEDC0430DD78E6F19A7DA3A27498A4181E24D738B0072D8F6ADB8C9809A5B033A09785814FD9919F6EF9F83EEA519BEC593855C4C10CBEEC582D4AE0792158823B0275E6AEC35242740468FAF3D5C60FD1E376362B6322F78B7ED0CA1C5BBCD2B49734A56C0967A1D01A100932C837B91D592CE08ABFF",
	],
	[
		"2",
		"0xB08FFB20760FFED58FADA86DFEF71AD72AA0FA763219618FE022C197E54708BB1191C66470250FCE8879487507CEE41381CA4D932F81C2B3F1AB20B539D50DCD",
		"0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF72", # even
		"0x7858794B5897C29F4ED0B40913416AB6C48588484E6A45F2ED3E26C941D878E923575AAC434EE2750E6439A6976F9BB4D64CEDB2A53CE8D04DD48CADCDF8E46F22747C6B81C6CEA86C0D873FBF7CEF262BAAC43A522BD7F32F3CDAC52B9337C77B3DCFB3DB3EDD80476331E82F4B1DF8EFDC1220C92656DFC9197BDC1877804E28D928A2A284B8DED506CBA304435C9D0133C246C98A7D890D1DE60CBC53A024361DA83A9B8775019083D22AC6820ED7C3C68F8E801DD4EC779EE0A05C6EB682EF9840D285B838369BA7E148FA27691D524FAEAF7C6ECE2A4B99A294B9F2C241857B5B90CC8BFFCFCF18DFA7D676131D5CD3855A5A3E8EBFA0CDFADB4D198B4A",
	],
]

func TestExp(t: TestingT) -> void:
	for i in len(expTests):
		var test := expTests[i]

		var x := BigInt.new()
		var y := BigInt.new()
		var err1 := x.SetString(test[0])
		var err2 := y.SetString(test[1])

		var err3: Error
		var err4: Error
		var out: BigInt
		var m: BigInt

		if len(test[3]) == 0:
			out = null
			err3 = OK
		else:
			out = BigInt.new()
			err3 = out.SetString(test[3])

		if len(test[2]) == 0:
			m = null
			err4 = OK
		else:
			m = BigInt.new()
			err4 = m.SetString(test[2])

		if err1 != OK or err2 != OK or err3 != OK or err4 != OK:
			t.Error("#%d: error in input" % [i])
			continue

		var z1 := BigInt.new()
		var err := z1.Exp(x, y, m)
		if err == OK and not isNormalized(z1):
			t.Error("#%d: %s is not normalized" % [i, z1])
		if not ((err != OK and out == null) or z1.Cmp(out) == 0):
			t.Error("#%d: got %s want %s" % [i, z1.String(16), out.String(16)])

		if m == null:
			# The result should be the same as for m == 0;
			# specifically, there should be no div-zero panic.
			m = BigInt.new() # m != nil && len(m.abs) == 0
			var z2 := BigInt.new()
			z2.Exp(x, y, m)
			if z2.Cmp(z1) != 0:
				t.Error("#%d: got %s want %s" % [i, z2.String(16), z1.String(16)])

func BenchmarkExp(b: TestingB) -> void:
	var x := BigInt.new()
	var y := BigInt.new()
	var n := BigInt.new()
	var out := BigInt.new()
	x.SetString("11001289118363089646017359372117963499250546375269047542777928006103246876688756735760905680604646624353196869572752623285140408755420374049317646428185270079555372763503115646054602867593662923894140940837479507194934267532831694565516466765025434902348314525627418515646588160955862839022051353653052947073136084780742729727874803457643848197499548297570026926927502505634297079527299004267769780768565695459945235586892627059178884998772989397505061206395455591503771677500931269477503508150175717121828518985901959919560700853226255420793148986854391552859459511723547532575574664944815966793196961286234040892865")
	y.SetString("0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF72")
	n.SetString("0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73")
	b.ResetTimer()
	for i in b.N:
		out.Exp(x, y, n)

func BenchmarkExpMont(b0: TestingB) -> void:
	var x := BigInt.new()
	var y := BigInt.new()
	x.SetString("297778224889315382157302278696111964193")
	y.SetString("2548977943381019743024248146923164919440527843026415174732254534318292492375775985739511369575861449426580651447974311336267954477239437734832604782764979371984246675241012538135715981292390886872929238062252506842498360562303324154310849745753254532852868768268023732398278338025070694508489163836616810661033068070127919590264734220833816416141878688318329193389865030063416339367925710474801991305827284114894677717927892032165200876093838921477120036402410731159852999623461591709308405270748511350289172153076023215")
	var mods: Array[Array] = [
		["Odd", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF"],
		["Even1", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FE"],
		["Even2", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FC"],
		["Even3", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281F8"],
		["Even4", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281F0"],
		["Even8", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B21828100"],
		["Even32", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B00000000"],
		["Even64", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828282828200FF0000000000000000"],
		["Even96", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF82828283000000000000000000000000"],
		["Even128", "0x82828282828200FFFF28FF2B218281FF82828282828200FFFF28FF2B218281FF00000000000000000000000000000000"],
		["Even255", "0x82828282828200FFFF28FF2B218281FF8000000000000000000000000000000000000000000000000000000000000000"],
		["SmallEven1", "0x7E"],
		["SmallEven2", "0x7C"],
		["SmallEven3", "0x78"],
		["SmallEven4", "0x70"],
	]
	for mod in mods:
		var n := BigInt.new()
		n.SetString(mod[1])
		var out := BigInt.new()
		b0.Run(mod[0], func(b: TestingB) -> void:
			for i in b.N:
				out.Exp(x, y, n))

func BenchmarkExp2(b: TestingB) -> void:
	var x := BigInt.new()
	var y := BigInt.new()
	var n := BigInt.new()
	var out := BigInt.new()
	x.SetString("2")
	y.SetString("0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF72")
	n.SetString("0xAC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73")
	b.ResetTimer()
	for i in b.N:
		out.Exp(x, y, n)

static func checkGcd(aBytes: PackedByteArray, bBytes: PackedByteArray) -> bool:
	var x := BigInt.new()
	var y := BigInt.new()
	var a := BigInt.new()
	var b := BigInt.new()
	a.SetBytes(aBytes)
	b.SetBytes(bBytes)

	var d := BigInt.new()
	d.GCD(x, y, a, b)
	x.Mul(x, a)
	y.Mul(y, b)
	x.Add(x, y)

	return x.Cmp(d) == 0

# euclidExtGCD is a reference implementation of Euclid's
# extended GCD algorithm for testing against optimized algorithms.
# Requirements: a, b > 0
static func euclidExtGCD(a: BigInt, b: BigInt) -> Array[BigInt]:
	var A := BigInt.new()
	var B := BigInt.new()
	A.Set(a)
	B.Set(b)

	# A = Ua*a + Va*b
	# B = Ub*a + Vb*b
	var Ua := BigInt.NewInt(1)
	var Va := BigInt.new()

	var Ub := BigInt.new()
	var Vb := BigInt.NewInt(1)

	var q := BigInt.new()
	var temp := BigInt.new()

	var r := BigInt.new()
	while not B._abs.is_empty():
		q.QuoRem(A, B, r)

		var swap := A
		A = B
		B = r
		r = swap

		# Ua, Ub = Ub, Ua-q*Ub
		temp.Set(Ub)
		Ub.Mul(Ub, q)
		Ub.Sub(Ua, Ub)
		Ua.Set(temp)

		# Va, Vb = Vb, Va-q*Vb
		temp.Set(Vb)
		Vb.Mul(Vb, q)
		Vb.Sub(Va, Vb)
		Va.Set(temp)

	return [A, Ua, Va]

static func checkLehmerGcd(aBytes: PackedByteArray, bBytes: PackedByteArray) -> bool:
	var a := BigInt.new()
	var b := BigInt.new()
	a.SetBytes(aBytes)
	b.SetBytes(bBytes)

	if a.Sign() <= 0 or b.Sign() <= 0:
		return true # can only test positive arguments

	var d := BigInt.new()
	d._lehmerGCD(null, null, a, b)
	var d0 := euclidExtGCD(a, b)

	return d.Cmp(d0[0]) == 0

static func checkLehmerExtGcd(aBytes: PackedByteArray, bBytes: PackedByteArray) -> bool:
	var a := BigInt.new()
	var b := BigInt.new()
	var x := BigInt.new()
	var y := BigInt.new()
	a.SetBytes(aBytes)
	b.SetBytes(bBytes)

	if a.Sign() <= 0 or b.Sign() <= 0:
		return true # can only test positive arguments

	var d := BigInt.new()
	d._lehmerGCD(x, y, a, b)
	var d0x0y0 := euclidExtGCD(a, b)

	return d.Cmp(d0x0y0[0]) == 0 and x.Cmp(d0x0y0[1]) == 0 and y.Cmp(d0x0y0[2]) == 0

const gcdTests: Array[Array] = [
	# a <= 0 || b <= 0
	["0", "0", "0", "0", "0"],
	["7", "0", "1", "0", "7"],
	["7", "0", "-1", "0", "-7"],
	["11", "1", "0", "11", "0"],
	["7", "-1", "-2", "-77", "35"],
	["935", "-3", "8", "64515", "24310"],
	["935", "-3", "-8", "64515", "-24310"],
	["935", "3", "-8", "-64515", "-24310"],

	["1", "-9", "47", "120", "23"],
	["7", "1", "-2", "77", "35"],
	["935", "-3", "8", "64515", "24310"],
	["935000000000000000", "-3", "8", "64515000000000000000", "24310000000000000000"],
	["1", "-221", "22059940471369027483332068679400581064239780177629666810348940098015901108344", "98920366548084643601728869055592650835572950932266967461790948584315647051443", "991"],
]

static func testGcd(t: TestingT, d: BigInt, x: BigInt, y: BigInt, a: BigInt, b: BigInt) -> void:
	var X: BigInt
	if x != null:
		X = BigInt.new()
	var Y: BigInt
	if y != null:
		Y = BigInt.new()

	var D := BigInt.new()
	D.GCD(X, Y, a, b)
	if D.Cmp(d) != 0:
		t.Error("GCD(%s, %s, %s, %s): got d = %s, want %s" % [x, y, a, b, D, d])
	if x != null and X.Cmp(x) != 0:
		t.Error("GCD(%s, %s, %s, %s): got x = %s, want %s" % [x, y, a, b, X, x])
	if y != null and Y.Cmp(y) != 0:
		t.Error("GCD(%s, %s, %s, %s): got y = %s, want %s" % [x, y, a, b, Y, y])

	# check results in presence of aliasing (issue #11284)
	var a2 := BigInt.new()
	var b2 := BigInt.new()
	a2.Set(a)
	b2.Set(b)
	a2.GCD(X, Y, a2, b2) # result is same as 1st argument
	if a2.Cmp(d) != 0:
		t.Error("aliased z = a GCD(%s, %s, %s, %s): got d = %s, want %s" % [x, y, a, b, a2, d])
	if x != null and X.Cmp(x) != 0:
		t.Error("aliased z = a GCD(%s, %s, %s, %s): got x = %s, want %s" % [x, y, a, b, X, x])
	if y != null and Y.Cmp(y) != 0:
		t.Error("aliased z = a GCD(%s, %s, %s, %s): got y = %s, want %s" % [x, y, a, b, Y, y])

	a2.Set(a)
	b2.Set(b)
	b2.GCD(X, Y, a2, b2) # result is same as 2nd argument
	if b2.Cmp(d) != 0:
		t.Error("aliased z = b GCD(%s, %s, %s, %s): got d = %s, want %s" % [x, y, a, b, b2, d])
	if x != null and X.Cmp(x) != 0:
		t.Error("aliased z = b GCD(%s, %s, %s, %s): got x = %s, want %s" % [x, y, a, b, X, x])
	if y != null and Y.Cmp(y) != 0:
		t.Error("aliased z = b GCD(%s, %s, %s, %s): got y = %s, want %s" % [x, y, a, b, Y, y])

	a2.Set(a)
	b2.Set(b)
	D.GCD(a2, b2, a2, b2) # x = a, y = b
	if D.Cmp(d) != 0:
		t.Error("aliased x = a, y = b GCD(%s, %s, %s, %s): got d = %s, want %s" % [x, y, a, b, D, d])
	if x != null and a2.Cmp(x) != 0:
		t.Error("aliased x = a, y = b GCD(%s, %s, %s, %s): got x = %s, want %s" % [x, y, a, b, a2, x])
	if y != null and b2.Cmp(y) != 0:
		t.Error("aliased x = a, y = b GCD(%s, %s, %s, %s): got y = %s, want %s" % [x, y, a, b, b2, y])

	a2.Set(a)
	b2.Set(b)
	D.GCD(b2, a2, a2, b2) # x = b, y = a
	if D.Cmp(d) != 0:
		t.Error("aliased x = b, y = a GCD(%s, %s, %s, %s): got d = %s, want %s" % [x, y, a, b, D, d])
	if x != null and b2.Cmp(x) != 0:
		t.Error("aliased x = b, y = a GCD(%s, %s, %s, %s): got x = %s, want %s" % [x, y, a, b, b2, x])
	if y != null and a2.Cmp(y) != 0:
		t.Error("aliased x = b, y = a GCD(%s, %s, %s, %s): got y = %s, want %s" % [x, y, a, b, a2, y])

func TestGcd(t: TestingT) -> void:
	for test in gcdTests:
		var d := BigInt.new()
		var x := BigInt.new()
		var y := BigInt.new()
		var a := BigInt.new()
		var b := BigInt.new()
		d.SetString(test[0])
		x.SetString(test[1])
		y.SetString(test[2])
		a.SetString(test[3])
		b.SetString(test[4])

		testGcd(t, d, null, null, a, b)
		testGcd(t, d, x, null, a, b)
		testGcd(t, d, null, y, a, b)
		testGcd(t, d, x, y, a, b)

	t.QuickCheckBB(checkGcd)
	t.QuickCheckBB(checkLehmerGcd)
	t.QuickCheckBB(checkLehmerExtGcd)

const rshTests: Array[Array] = [
	["0", 0, "0"],
	["-0", 0, "0"],
	["0", 1, "0"],
	["0", 2, "0"],
	["1", 0, "1"],
	["1", 1, "0"],
	["1", 2, "0"],
	["2", 0, "2"],
	["2", 1, "1"],
	["-1", 0, "-1"],
	["-1", 1, "-1"],
	["-1", 10, "-1"],
	["-100", 2, "-25"],
	["-100", 3, "-13"],
	["-100", 100, "-1"],
	["4294967296", 0, "4294967296"],
	["4294967296", 1, "2147483648"],
	["4294967296", 2, "1073741824"],
	["18446744073709551616", 0, "18446744073709551616"],
	["18446744073709551616", 1, "9223372036854775808"],
	["18446744073709551616", 2, "4611686018427387904"],
	["18446744073709551616", 64, "1"],
	["340282366920938463463374607431768211456", 64, "18446744073709551616"],
	["340282366920938463463374607431768211456", 128, "1"],
]

func TestRsh(t: TestingT) -> void:
	for i in len(rshTests):
		var test := rshTests[i]
		var in_ := BigInt.new()
		var expected := BigInt.new()
		var out := BigInt.new()
		in_.SetString(test[0], 10)
		expected.SetString(test[2], 10)
		out.Rsh(in_, test[1])

		if not isNormalized(out):
			t.Error("#%d: %s is not normalized" % [i, out])
		if out.Cmp(expected) != 0:
			t.Error("#%d: got %s want %s" % [i, out, expected])

func TestRshSelf(t: TestingT) -> void:
	for i in len(rshTests):
		var test := rshTests[i]
		var z := BigInt.new()
		var expected := BigInt.new()
		z.SetString(test[0], 10)
		expected.SetString(test[2], 10)
		z.Rsh(z, test[1])

		if not isNormalized(z):
			t.Error("#%d: %s is not normalized" % [i, z])
		if z.Cmp(expected) != 0:
			t.Error("#%d: got %s want %s" % [i, z, expected])

const lshTests: Array[Array] = [
	["0", 0, "0"],
	["0", 1, "0"],
	["0", 2, "0"],
	["1", 0, "1"],
	["1", 1, "2"],
	["1", 2, "4"],
	["2", 0, "2"],
	["2", 1, "4"],
	["2", 2, "8"],
	["-87", 1, "-174"],
	["4294967296", 0, "4294967296"],
	["4294967296", 1, "8589934592"],
	["4294967296", 2, "17179869184"],
	["18446744073709551616", 0, "18446744073709551616"],
	["9223372036854775808", 1, "18446744073709551616"],
	["4611686018427387904", 2, "18446744073709551616"],
	["1", 64, "18446744073709551616"],
	["18446744073709551616", 64, "340282366920938463463374607431768211456"],
	["1", 128, "340282366920938463463374607431768211456"],
]

func TestLsh(t: TestingT) -> void:
	for i in len(lshTests):
		var test := lshTests[i]
		var in_ := BigInt.new()
		var expected := BigInt.new()
		var out := BigInt.new()
		in_.SetString(test[0], 10)
		expected.SetString(test[2], 10)
		out.Lsh(in_, test[1])

		if not isNormalized(out):
			t.Error("#%d: %s is not normalized" % [i, out])
		if out.Cmp(expected) != 0:
			t.Error("#%d: got %s want %s" % [i, out, expected])

func TestLshSelf(t: TestingT) -> void:
	for i in len(lshTests):
		var test := lshTests[i]
		var z := BigInt.new()
		var expected := BigInt.new()
		z.SetString(test[0], 10)
		expected.SetString(test[2], 10)
		z.Lsh(z, test[1])

		if not isNormalized(z):
			t.Error("#%d: %s is not normalized" % [i, z])
		if z.Cmp(expected) != 0:
			t.Error("#%d: got %s want %s" % [i, z, expected])

func TestLshRsh(t: TestingT) -> void:
	for i in len(rshTests):
		var test := rshTests[i]
		var in_ := BigInt.new()
		var out := BigInt.new()
		in_.SetString(test[0], 10)
		out.Lsh(in_, test[1])
		out.Rsh(out, test[1])

		if not isNormalized(out):
			t.Error("#%d: %s is not normalized" % [i, out])
		if in_.Cmp(out) != 0:
			t.Error("#%d: got %s want %s" % [i, out, in_])

	for i in len(lshTests):
		var test := lshTests[i]
		var in_ := BigInt.new()
		var out := BigInt.new()
		in_.SetString(test[0], 10)
		out.Lsh(in_, test[1])
		out.Rsh(out, test[1])

		if not isNormalized(out):
			t.Error("#%d: %s is not normalized" % [i, out])
		if in_.Cmp(out) != 0:
			t.Error("#%d: got %s want %s" % [i, out, in_])

# Entries must be sorted by value in ascending order.
const cmpAbsTests: PackedStringArray = [
	"0",
	"1",
	"2",
	"10",
	"10000000",
	"2783678367462374683678456387645876387564783686583485",
	"2783678367462374683678456387645876387564783686583486",
	"32957394867987420967976567076075976570670947609750670956097509670576075067076027578341538",
]

func TestCmpAbs(t: TestingT) -> void:
	var values: Array[BigInt]
	values.resize(len(cmpAbsTests))
	var prev: BigInt = null
	for i in len(cmpAbsTests):
		var s: String = cmpAbsTests[i]
		var x := BigInt.new()
		var err := x.SetString(s)
		if err != OK:
			t.Error("SetString(%s, 0) failed" % [s])
			return
		if prev != null and prev.Cmp(x) >= 0:
			t.Error("cmpAbsTests entries not sorted in ascending order")
			return
		values[i] = x
		prev = x

	for i in len(values):
		var x := values[i]
		for j in len(values):
			var y := values[j]
			# try all combinations of signs for x, y
			for k in 4:
				var a := BigInt.new()
				var b := BigInt.new()
				a.Set(x)
				b.Set(y)
				if (k & 1) != 0:
					a.Neg(a)
				if (k & 2) != 0:
					b.Neg(b)

				var got := a.CmpAbs(b)
				var want := 0
				if i > j:
					want = 1
				elif i < j:
					want = -1
				if got != want:
					t.Error("absCmp |%s|, |%s|: got %d; want %d" % [a, b, got, want])

func TestIntCmpSelf(t: TestingT) -> void:
	for s in cmpAbsTests:
		var x := BigInt.new()
		var err := x.SetString(s)
		if err != OK:
			t.Error("SetString(%s, 0) failed" % [s])
			return
		var got := x.Cmp(x)
		var want := 0
		if got != want:
			t.Errorf("x = %s: x.Cmp(x): got %d; want %d" % [x, got, want])

const int64Tests: Array[Array] = [
	# int64
	["0", 0],
	["1", 1],
	["-1", -1],
	["4294967295", 4294967295],
	["-4294967295", -4294967295],
	["4294967296", 4294967296],
	["-4294967296", -4294967296],
	["9223372036854775807", 9223372036854775807],
	["-9223372036854775807", -9223372036854775807],
	["-9223372036854775808", -9223372036854775808],

	# not int64
	["0x8000000000000000", null],
	["-0x8000000000000001", null],
	["38579843757496759476987459679745", null],
	["-38579843757496759476987459679745", null],
]

func TestInt64(t: TestingT) -> void:
	for test in int64Tests:
		var x := BigInt.new()
		var err := x.SetString(test[0])
		if err != OK:
			t.Error("SetString(%s, 0) failed" % [test[0]])
			continue

		if test[1] == null:
			if x.IsInt64():
				t.Error("IsInt64(%s) succeeded unexpectedly" % [test[0]])
			continue

		if not x.IsInt64():
			t.Error("IsInt64(%s) failed unexpectedly" % [test[0]])

		var got := x.Int64()
		var want: int = test[1]
		if got != want:
			t.Error("Int64(%s) = %d; want %d" % [test[0], got, want])

const uint64Tests: Array[Array] = [
	# uint64
	["0", 0],
	["1", 1],
	["4294967295", 4294967295],
	["4294967296", 4294967296],
	["8589934591", 8589934591],
	["8589934592", 8589934592],
	["9223372036854775807", 9223372036854775807],
	["9223372036854775808", -9223372036854775808],
	["0x08000000000000000", -0x08000000000000000],

	# not uint64
	["0x10000000000000000", null],
	["-0x08000000000000000", null],
	["-1", null],
]

func TestUint64(t: TestingT) -> void:
	for test in uint64Tests:
		var x := BigInt.new()
		var err := x.SetString(test[0])
		if err != OK:
			t.Error("SetString(%s, 0) failed" % [test[0]])
			continue

		if test[1] == null:
			if x.IsUint64():
				t.Error("IsUint64(%s) succeeded unexpectedly" % [test[0]])
			continue

		if not x.IsUint64():
			t.Error("IsUint64(%s) failed unexpectedly" % [test[0]])

		var got := x.Uint64()
		var want: int = test[1]
		if got != want:
			t.Error("Uint64(%s) = %d; want %d" % [test[0], got, want])

const bitwiseTests: Array[Array] = [
	["0x00", "0x00", "0x00", "0x00", "0x00", "0x00"],
	["0x00", "0x01", "0x00", "0x01", "0x01", "0x00"],
	["0x01", "0x00", "0x00", "0x01", "0x01", "0x01"],
	["-0x01", "0x00", "0x00", "-0x01", "-0x01", "-0x01"],
	["-0xaf", "-0x50", "-0xf0", "-0x0f", "0xe1", "0x41"],
	["0x00", "-0x01", "0x00", "-0x01", "-0x01", "0x00"],
	["0x01", "0x01", "0x01", "0x01", "0x00", "0x00"],
	["-0x01", "-0x01", "-0x01", "-0x01", "0x00", "0x00"],
	["0x07", "0x08", "0x00", "0x0f", "0x0f", "0x07"],
	["0x05", "0x0f", "0x05", "0x0f", "0x0a", "0x00"],
	["0xff", "-0x0a", "0xf6", "-0x01", "-0xf7", "0x09"],
	["0x013ff6", "0x9a4e", "0x1a46", "0x01bffe", "0x01a5b8", "0x0125b0"],
	["-0x013ff6", "0x9a4e", "0x800a", "-0x0125b2", "-0x01a5bc", "-0x01c000"],
	["-0x013ff6", "-0x9a4e", "-0x01bffe", "-0x1a46", "0x01a5b8", "0x8008"],
	[
		"0x1000009dc6e3d9822cba04129bcbe3401",
		"0xb9bd7d543685789d57cb918e833af352559021483cdb05cc21fd",
		"0x1000001186210100001000009048c2001",
		"0xb9bd7d543685789d57cb918e8bfeff7fddb2ebe87dfbbdfe35fd",
		"0xb9bd7d543685789d57ca918e8ae69d6fcdb2eae87df2b97215fc",
		"0x8c40c2d8822caa04120b8321400",
	],
	[
		"0x1000009dc6e3d9822cba04129bcbe3401",
		"-0xb9bd7d543685789d57cb918e833af352559021483cdb05cc21fd",
		"0x8c40c2d8822caa04120b8321401",
		"-0xb9bd7d543685789d57ca918e82229142459020483cd2014001fd",
		"-0xb9bd7d543685789d57ca918e8ae69d6fcdb2eae87df2b97215fe",
		"0x1000001186210100001000009048c2000",
	],
	[
		"-0x1000009dc6e3d9822cba04129bcbe3401",
		"-0xb9bd7d543685789d57cb918e833af352559021483cdb05cc21fd",
		"-0xb9bd7d543685789d57cb918e8bfeff7fddb2ebe87dfbbdfe35fd",
		"-0x1000001186210100001000009048c2001",
		"0xb9bd7d543685789d57ca918e8ae69d6fcdb2eae87df2b97215fc",
		"0xb9bd7d543685789d57ca918e82229142459020483cd2014001fc",
	],
]

static func testBitFun(t: TestingT, msg: String, f: Callable, x: BigInt, y: BigInt, exp_: String) -> void:
	var expected := BigInt.new()
	expected.SetString(exp_)

	var out := BigInt.new()
	f.call(out, x, y)
	if out.Cmp(expected) != 0:
		t.Error("%s: got %s want %s" % [msg, out, expected])

static func testBitFunSelf(t: TestingT, msg: String, f: Callable, x: BigInt, y: BigInt, exp_: String) -> void:
	var z := BigInt.new()
	z.Set(x)
	var expected := BigInt.new()
	expected.SetString(exp_)

	f.call(z, z, y)
	if z.Cmp(expected) != 0:
		t.Error("%s: got %s want %s" % [msg, z, expected])

static func altBit(x: BigInt, i: int) -> int:
	var z := BigInt.new()
	z.Rsh(x, i)
	z.And(z, BigInt.NewInt(1))
	if z.Cmp(BigInt.new()) != 0:
		return 1
	return 0

static func altSetBit(z: BigInt, x: BigInt, i: int, b: int) -> void:
	var m := BigInt.NewInt(1)
	m.Lsh(m, i)
	assert(b == 0 or b == 1)
	if b == 1:
		z.Or(x, m)
	elif b == 0:
		z.AndNot(x, m)

static func testBitset(t: TestingT, x: BigInt) -> void:
	var n := x.BitLen()
	for i in n + 10:
		var old := x.Bit(i)
		var old1 := altBit(x, i)
		if old != old1:
			t.Error("bitset: inconsistent value for Bit(%s, %d), got %d want %d" % [x, i, old, old1])
		var z := BigInt.new()
		var z1 := BigInt.new()
		z.SetBit(x, i, 1)
		altSetBit(z1, x, i, 1)
		if z.Bit(i) == 0:
			t.Error("bitset: bit %d of %s got 0 want 1" % [i, x])
		if z.Cmp(z1) != 0:
			t.Error("bitset: inconsistent value after SetBit 1, got %s want %s" % [z, z1])
		z.SetBit(z, i, 0)
		altSetBit(z1, z1, i, 0)
		if z.Bit(i) != 0:
			t.Error("bitset: bit %d of %s got 1 want 0" % [i, x])
		if z.Cmp(z1) != 0:
			t.Error("bitset: inconsistent value after SetBit 0, got %s want %s" % [z, z1])
		altSetBit(z1, z1, i, old)
		z.SetBit(z, i, old)
		if z.Cmp(z1) != 0:
			t.Error("bitset: inconsistent value after SetBit old, got %s want %s" % [z, z1])

const bitsetTests: Array[Array] = [
	["0", 0, 0],
	["0", 200, 0],
	["1", 0, 1],
	["1", 1, 0],
	["-1", 0, 1],
	["-1", 200, 1],
	["0x2000000000000000000000000000", 108, 0],
	["0x2000000000000000000000000000", 109, 1],
	["0x2000000000000000000000000000", 110, 0],
	["-0x2000000000000000000000000001", 108, 1],
	["-0x2000000000000000000000000001", 109, 0],
	["-0x2000000000000000000000000001", 110, 1],
]

func TestBitSet(t: TestingT) -> void:
	for test in bitwiseTests:
		var x := BigInt.new()
		x.SetString(test[0])
		testBitset(t, x)

		var y := BigInt.new()
		y.SetString(test[1])
		testBitset(t, y)

	for i in len(bitsetTests):
		var test := bitsetTests[i]
		var x := BigInt.new()
		x.SetString(test[0])

		var b := x.Bit(test[1])
		if b != test[2]:
			t.Error("#%d got %d want %d" % [i, b, test[2]])

	var z := BigInt.NewInt(1)
	z.SetBit(BigInt.NewInt(0), 2, 1)
	if z.Cmp(BigInt.NewInt(4)) != 0:
		t.Error("destination leaked into result; got %s want 4" % [z])

const tzbTests: Array[Array] = [
	["0", 0],
	["1", 0],
	["-1", 0],
	["4", 2],
	["-8", 3],
	["0x4000000000000000000", 74],
	["-0x8000000000000000000", 75],
]

func TestTrailingZeroBits(t: TestingT) -> void:
	for i in len(tzbTests):
		var test := tzbTests[i]

		var x := BigInt.new()
		x.SetString(test[0])
		var want: int = test[1]
		var got := x.TrailingZeroBits()

		if got != want:
			t.Error("#%d: got %d want %d" % [i, got, want])

func BenchmarkBitset(b: TestingB) -> void:
	var z := BigInt.new()
	z.SetBit(z, 512, 1)
	b.ResetTimer()
	for i in b.N:
		z.SetBit(z, i & 512, 1)

func BenchmarkBitsetNeg(b: TestingB) -> void:
	var z := BigInt.NewInt(-1)
	z.SetBit(z, 512, 0)
	b.ResetTimer()
	for i in b.N:
		z.SetBit(z, i & 512, 0)

func BenchmarkBitsetOrig(b: TestingB) -> void:
	var z := BigInt.new()
	altSetBit(z, z, 512, 1)
	b.ResetTimer()
	for i in b.N:
		altSetBit(z, z, i & 512, 1)

func BenchmarkBitsetNegOrig(b: TestingB) -> void:
	var z := BigInt.NewInt(-1)
	altSetBit(z, z, 512, 0)
	b.ResetTimer()
	for i in b.N:
		altSetBit(z, z, i & 512, 0)

# tri generates the trinomial 2**(n*2) - 2**n - 1, which is always 3 mod 4 and
# 7 mod 8, so that 2 is always a quadratic residue.
static func tri(n: int) -> BigInt:
	var x := BigInt.NewInt(1)
	x.Lsh(x, n)
	var x2 := BigInt.new()
	x2.Lsh(x, n)
	x2.Sub(x2, x)
	x2.Sub(x2, BigInt.NewInt(1))
	return x2

func BenchmarkModSqrt225_Tonelli(b: TestingB) -> void:
	var p := tri(225)
	var x := BigInt.NewInt(2)
	b.ResetTimer()
	for i in b.N:
		x.SetUint64(2)
		x._modSqrtTonelliShanks(x, p)

func BenchmarkModSqrt225_3Mod4(b: TestingB) -> void:
	var p := tri(225)
	var x := BigInt.NewInt(2)
	b.ResetTimer()
	for i in b.N:
		x.SetUint64(2)
		x._modSqrt3Mod4Prime(x, p)

func BenchmarkModSqrt231_Tonelli(b: TestingB) -> void:
	var p := tri(231)
	p.Sub(p, BigInt.NewInt(2)) # tri(231) - 2 is a prime == 5 mod 8
	var x := BigInt.NewInt(7)
	b.ResetTimer()
	for i in b.N:
		x.SetUint64(7)
		x._modSqrtTonelliShanks(x, p)

func BenchmarkModSqrt231_5Mod8(b: TestingB) -> void:
	var p := tri(231)
	p.Sub(p, BigInt.NewInt(2)) # tri(231) - 2 is a prime == 5 mod 8
	var x := BigInt.NewInt(7)
	b.ResetTimer()
	for i in b.N:
		x.SetUint64(7)
		x._modSqrt5Mod8Prime(x, p)

func TestBitwise(t: TestingT) -> void:
	var x0 := BigInt.new()
	var y0 := BigInt.new()
	for test in bitwiseTests:
		x0.SetString(test[0])
		y0.SetString(test[1])

		testBitFun(t, "and", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.And(x, y), x0, y0, test[2])
		testBitFunSelf(t, "and", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.And(x, y), x0, y0, test[2])
		testBitFun(t, "andNot", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.AndNot(x, y), x0, y0, test[5])
		testBitFunSelf(t, "andNot", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.AndNot(x, y), x0, y0, test[5])
		testBitFun(t, "or", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.Or(x, y), x0, y0, test[3])
		testBitFunSelf(t, "or", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.Or(x, y), x0, y0, test[3])
		testBitFun(t, "xor", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.Xor(x, y), x0, y0, test[4])
		testBitFunSelf(t, "xor", func(z: BigInt, x: BigInt, y: BigInt) -> void: z.Xor(x, y), x0, y0, test[4])

const notTests: Array[Array] = [
	["0", "-1"],
	["1", "-2"],
	["7", "-8"],
	["0", "-1"],
	["-81910", "81909"],
	[
		"298472983472983471903246121093472394872319615612417471234712061",
		"-298472983472983471903246121093472394872319615612417471234712062",
	],
]

func TestNot(t: TestingT) -> void:
	var in_ := BigInt.new()
	var out := BigInt.new()
	var expected := BigInt.new()
	for i in len(notTests):
		var test := notTests[i]

		in_.SetString(test[0], 10)
		expected.SetString(test[1], 10)

		out.Not(in_)

		if out.Cmp(expected) != 0:
			t.Error("#%d: got %s want %s" % [i, out, expected])

		out.Not(out)
		if out.Cmp(in_) != 0:
			t.Errorf("#%d: got %s want %s" % [i, out, in_])

const modInverseTests: Array[Array] = [
	["1234567", "458948883992"],
	["239487239847", "2410312426921032588552076022197566074856950548502459942654116941958108831682612228890093858261341614673227141477904012196503648957050582631942730706805009223062734745341073406696246014589361659774041027169249453200378729434170325843778659198143763193776859869524088940195577346119843545301547043747207749969763750084308926339295559968882457872412993810129130294592999947926365264059284647209730384947211681434464714438488520940127459844288859336526896320919633919"],
	["-10", "13"], # issue #16984
	["10", "-13"],
	["-17", "-13"],
]

func TestModInverse(t: TestingT) -> void:
	var element := BigInt.new()
	var modulus := BigInt.new()
	var gcd := BigInt.new()
	var inverse := BigInt.new()
	var one = BigInt.NewInt(1)

	for test in modInverseTests:
		element.SetString(test[0], 10)
		modulus.SetString(test[1], 10)
		inverse.ModInverse(element, modulus)
		inverse.Mul(inverse, element)
		inverse.Mod(inverse, modulus)
		if inverse.Cmp(one) != 0:
			t.Error("ModInverse(%s,%s)*%s%%%s=%s, not 1" % [element, modulus, element, modulus, inverse])

	# exhaustive test for small values
	for n in range(2, 100):
		modulus.SetInt64(n)
		for x in range(1, n):
			element.SetInt64(x)
			gcd.GCD(null, null, element, modulus)
			if gcd.Cmp(one) != 0:
				continue

			inverse.ModInverse(element, modulus)
			inverse.Mul(inverse, element)
			inverse.Mod(inverse, modulus)
			if inverse.Cmp(one) != 0:
				t.Error("ModInverse(%s,%s)*%s%%%s=%s, not 1" % [element, modulus, element, modulus, inverse])

func BenchmarkModInverse(b: TestingB) -> void:
	var p := BigInt.NewInt(1) # Mersenne prime 2**1279 -1
	p.Lsh(p, 1279)
	p.Sub(p, BigInt.NewInt(1))
	var x := BigInt.new()
	x.Sub(p, BigInt.NewInt(1))
	var z := BigInt.new()
	b.ResetTimer()
	for i in b.N:
		z.ModInverse(x, p)

# testModSqrt is a helper for TestModSqrt,
# which checks that ModSqrt can compute a square-root of elt^2.
static func testModSqrt(t: TestingT, elt: BigInt, mod: BigInt, sq: BigInt, sqrt_: BigInt) -> bool:
	var sqChk := BigInt.new()
	var sqrtChk := BigInt.new()
	var sqrtsq := BigInt.new()
	sq.Mul(elt, elt)
	sq.Mod(sq, mod)
	sqrt_.ModSqrt(sq, mod)

	# test ModSqrt arguments outside the range [0,mod)
	sqChk.Add(sq, mod)
	var err := sqrtChk.ModSqrt(sqChk, mod)
	if err != OK or sqrtChk.Cmp(sqrt_) != 0:
		t.Error("ModSqrt returned inconsistent value %s" % [sqrtChk])

	sqChk.Sub(sq, mod)
	err = sqrtChk.ModSqrt(sqChk, mod)
	if err != OK or sqrtChk.Cmp(sqrt_) != 0:
		t.Error("ModSqrt returned inconsistent value %s" % [sqrtChk])

	# test x aliasing z
	sqrtChk.Set(sq)
	err = sqrtChk.ModSqrt(sqrtChk, mod)
	if err != OK or sqrtChk.Cmp(sqrt_) != 0:
		t.Error("ModSqrt returned inconsistent value %s" % [sqrtChk])

	# make sure we actually got a square root
	if sqrt_.Cmp(elt) == 0:
		return true # we found the "desired" square root

	sqrtsq.Mul(sqrt_, sqrt_) # make sure we found the "other" one
	sqrtsq.Mod(sqrtsq, mod)
	return sq.Cmp(sqrtsq) == 0

func TestModSqrt(t: TestingT) -> void:
	var elt := BigInt.new()
	var mod := BigInt.new()
	var modx4 := BigInt.new()
	var sq := BigInt.new()
	var sqrt_ := BigInt.new()

	var r := RandomNumberGenerator.new()
	r.seed = 9

	for i in range(1, len(PrimeTest.primes)): # skip 2, use only odd primes
		var s := PrimeTest.primes[i]
		mod.SetString(s, 10)
		modx4.Lsh(mod, 2)

		# test a few random elements per prime
		for x in range(1, 5):
			elt.Rand(r.randi, modx4)
			elt.Sub(elt, mod) # test range [-mod, 3*mod)
			if not testModSqrt(t, elt, mod, sq, sqrt_):
				t.Error("#%d: failed (sqrt(e) = %s)" % [i, sqrt_])

	# exhaustive test for small values
	for n in range(3, 100):
		mod.SetInt64(n)
		if not mod.ProbablyPrime(10):
			continue

		var isSquare: Array[bool]
		isSquare.resize(n)

		# test all the squares
		for x in range(1, n):
			elt.SetInt64(x)
			if not testModSqrt(t, elt, mod, sq, sqrt_):
				t.Error("#%d: failed (sqrt(%s,%s) = %s)" % [x, elt, mod, sqrt_])

			isSquare[sq.Uint64()] = true

		# test all non-squares
		for x in range(1, n):
			sq.SetInt64(x)
			var err := sqrt_.ModSqrt(sq, mod)
			if not isSquare[x] and err == OK:
				t.Error("#%d: failed (sqrt(%s,%s) = nil)" % [x, sqrt_, mod])

func TestJacobi(t: TestingT) -> void:
	const testCases: Array[Array] = [
		[0, 1, 1],
		[0, -1, 1],
		[1, 1, 1],
		[1, -1, 1],
		[0, 5, 0],
		[1, 5, 1],
		[2, 5, -1],
		[-2, 5, -1],
		[2, -5, -1],
		[-2, -5, 1],
		[3, 5, -1],
		[5, 5, 0],
		[-5, 5, 0],
		[6, 5, 1],
		[6, -5, 1],
		[-6, 5, 1],
		[-6, -5, -1],
	]

	var x := BigInt.new()
	var y := BigInt.new()

	for i in len(testCases):
		var test := testCases[i]
		x.SetInt64(test[0])
		y.SetInt64(test[1])

		var expected: int = test[2]
		var actual := BigInt.Jacobi(x, y)
		if actual != expected:
			t.Errorf("#%d: Jacobi(%d, %d) = %d, but expected %d", i, test[0], test[1], actual, expected)

func TestIssue2607(_t: TestingT) -> void:
	# This code sequence used to hang.
	var n := BigInt.NewInt(10)
	var rand := RandomNumberGenerator.new()
	rand.seed = 9
	n.Rand(rand.randi, n)

func TestSqrt(t: TestingT) -> void:
	var root := 0
	var r := BigInt.new()
	for i in 10000:
		if (root + 1) * (root + 1) <= i:
			root += 1
		var n := BigInt.NewInt(i)
		r.SetInt64(-2)
		r.Sqrt(n)
		if r.Cmp(BigInt.NewInt(root)) != 0:
			t.Error("Sqrt(%s) = %s, want %d" % [n, r, root])

	for i in range(0, 1000, 10):
		var n := BigInt.new()
		n.SetString("1" + "0".repeat(i), 10)
		r.Sqrt(n)
		var r2 := BigInt.new()
		r2.SetString("1" + "0".repeat(i >> 1), 10)
		if r.Cmp(r2) != 0:
			t.Error("Sqrt(1e%d) = %s, want 1e%d" % [i, r, i >> 1])

	# Test aliasing.
	r.SetInt64(100)
	r.Sqrt(r)
	if r.Int64() != 10:
		t.Errorf("Sqrt(100) = %d, want 10 (aliased output)" % [r.Int64()])

# We can't test this together with the other Exp tests above because
# it requires a different receiver setup.
func TestIssue22830(t: TestingT) -> void:
	var one := BigInt.NewInt(1)
	var base := BigInt.new()
	var mod := BigInt.new()
	var want := BigInt.new()
	base.SetString("84555555300000000000", 10)
	mod.SetString("66666670001111111111", 10)
	want.SetString("17888885298888888889", 10)

	for n in [0, 1, -1]:
		var got := BigInt.NewInt(n)
		got.Exp(base, one, mod)
		if got.Cmp(want) != 0:
			t.Error("(%d).Exp(%s, 1, %s) = %s, want %s" % [n, base, mod, got, want])

func BenchmarkSqrt(b: TestingB) -> void:
	var n := BigInt.new()
	n.SetString("1" + "0".repeat(1001), 10)
	var t := BigInt.new()
	b.ResetTimer()
	for i in b.N:
		t.Sqrt(n)

static func benchmarkIntSqr(b: TestingB, nwords: int) -> void:
	var rand := RandomNumberGenerator.new()
	rand.seed = 1
	var x := BigInt.new()
	x._abs = NatTest.rndNat(rand, nwords)
	var t := BigInt.new()
	b.ResetTimer()
	for i in b.N:
		t.Mul(x, x)

func BenchmarkIntSqr(b0: TestingB) -> void:
	for n in NatTest.sqrBenchSizes:
		b0.Run("%d" % [n], benchmarkIntSqr.bind(n))

static func benchmarkDiv(b: TestingB, aSize: int, bSize: int) -> void:
	var r := RandomNumberGenerator.new()
	r.seed = 1234

	var aa := GCDTest.randInt(r.nexti, aSize)
	var bb := GCDTest.randInt(r.nexti, bSize)
	if aa.Cmp(bb) < 0:
		var swap := aa
		aa = bb
		bb = swap

	var x := BigInt.new()
	var y := BigInt.new()

	b.ResetTimer()
	for i in b.N:
		x.DivMod(aa, bb, y)

func BenchmarkDiv(b0: TestingB) -> void:
	for i: int in [
		10, 20, 50, 100, 200, 500, 1000,
		10000, 100000, 1000000, 10000000,
	]:
		var j := 2 * i
		b0.Run("%d/%d" % [j, i], benchmarkDiv.bind(j, i))

func TestNewIntMinInt64(t: TestingT) -> void:
	# Test for uint64 cast in NewInt.
	var want := INT64_MIN
	var got := BigInt.NewInt(want).Int64()
	if got != want:
		t.Error("wanted %d, got %d" % [want, got])

func TestFloat64(t: TestingT) -> void:
	# Godot parses -4503599627370495.000000 as -4503599627370494.500000
	var godot_parser_bug_workaround: PackedByteArray
	godot_parser_bug_workaround.resize(8)
	godot_parser_bug_workaround.encode_u32(0, 0xfffffffe)
	godot_parser_bug_workaround.encode_u32(4, 0xc32fffff)

	for test in [
		["-1000000000000000000000000000000000000000000000000000000", -1000000000000000078291540404596243842305360299886116864.000000, BigFloat.ACC_BELOW],
		["-9223372036854775809", INT64_MIN, BigFloat.ACC_ABOVE],
		["-9223372036854775808", -9223372036854775808, BigFloat.ACC_EXACT], # -2^63
		["-9223372036854775807", -9223372036854775807, BigFloat.ACC_BELOW],
		["-18014398509481985", -18014398509481984.000000, BigFloat.ACC_ABOVE],
		["-18014398509481984", -18014398509481984.000000, BigFloat.ACC_EXACT], # -2^54
		["-18014398509481983", -18014398509481984.000000, BigFloat.ACC_BELOW],
		["-9007199254740993", -9007199254740992.000000, BigFloat.ACC_ABOVE],
		["-9007199254740992", -9007199254740992.000000, BigFloat.ACC_EXACT], # -2^53
		["-9007199254740991", -9007199254740991.000000, BigFloat.ACC_EXACT],
		["-4503599627370497", -4503599627370497.000000, BigFloat.ACC_EXACT],
		["-4503599627370496", -4503599627370496.000000, BigFloat.ACC_EXACT], # -2^52
		["-4503599627370495", godot_parser_bug_workaround.decode_double(0), BigFloat.ACC_EXACT],
		["-12345", -12345, BigFloat.ACC_EXACT],
		["-1", -1, BigFloat.ACC_EXACT],
		["0", 0, BigFloat.ACC_EXACT],
		["1", 1, BigFloat.ACC_EXACT],
		["12345", 12345, BigFloat.ACC_EXACT],
		["0x1010000000000000", 0x1010000000000000, BigFloat.ACC_EXACT], # >2^53 but exact nonetheless
		["9223372036854775807", 9223372036854775808.0, BigFloat.ACC_ABOVE],
		["9223372036854775808", 9223372036854775808.0, BigFloat.ACC_EXACT], # +2^63
		["1000000000000000000000000000000000000000000000000000000", 1000000000000000078291540404596243842305360299886116864.000000, BigFloat.ACC_ABOVE],
	]:
		var i := BigInt.new()
		var err := i.SetString(test[0])
		if err != OK:
			t.Error("SetString(%s) failed" % [test[0]])
			continue

		# Test against expectation.
		var f := i.Float64()
		var acc := i.Float64Accuracy()
		if f != test[1] or acc != test[2]:
			t.Error("%s: got %f (%s); want %f (%s)" % [test[0], f, FloatTest.accuracyName[acc], test[1], FloatTest.accuracyName[test[2]]])

		# Cross-check the fast path against the big.Float implementation.
		var bf := BigFloat.new()
		bf.SetInt(i)
		var f2 := bf.Float64()
		var acc2 := bf.Float64Accuracy()
		if f != f2 or acc != acc2:
			t.Error("%s: got %f (%s); Float.Float64 gives %f (%s)" % [test[0], f, FloatTest.accuracyName[acc], f2, FloatTest.accuracyName[acc2]])
