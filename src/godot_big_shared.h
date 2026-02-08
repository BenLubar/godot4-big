#pragma once

#include <cstdint>
#include <godot_cpp/variant/packed_int64_array.hpp>

#ifndef GODOT_BIG_BASE_CLASS
#include <godot_cpp/classes/resource.hpp>
#define GODOT_BIG_BASE_CLASS godot::Resource
#elif defined(GODOT_BIG_BASE_CLASS_INCLUDE)
#include GODOT_BIG_BASE_CLASS_INCLUDE
#endif

enum BigAccuracy : int8_t {
	ACCURACY_BELOW = -1,
	ACCURACY_EXACT = 0,
	ACCURACY_ABOVE = +1,
};
