@tool
extends EditorScript

func _run() -> void:
	var sqrt2 := BigFloat.new()
	sqrt2.SetPrec(200)
	sqrt2.Sqrt(BigFloat.make(2))
	print("The square root of 2 is ", sqrt2)
