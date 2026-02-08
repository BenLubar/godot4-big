#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

#include "godot_big_int.h"
#include "godot_big_rat.h"
#include "godot_big_float.h"
#include "godot_big_naturals.h"

using namespace godot;

PackedInt64Array *natOne = nullptr;
PackedInt64Array *natTwo = nullptr;
PackedInt64Array *natThree = nullptr;
PackedInt64Array *natFive = nullptr;
PackedInt64Array *natTen = nullptr;
Ref<BigInt> *intOne = nullptr;
Ref<BigFloat> *floatThree = nullptr;
Vector<BigDivisor> *cacheBase10 = nullptr;
std::mutex cacheBase10Mutex;

void initialize_big_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	GDREGISTER_CLASS(BigInt);
	GDREGISTER_CLASS(BigRat);
	GDREGISTER_CLASS(BigFloat);

	natOne = memnew(PackedInt64Array);
	nat_setUint64(*natOne, 1);

	natTwo = memnew(PackedInt64Array);
	nat_setUint64(*natTwo, 2);

	natThree = memnew(PackedInt64Array);
	nat_setUint64(*natThree, 3);

	natFive = memnew(PackedInt64Array);
	nat_setUint64(*natFive, 5);

	natTen = memnew(PackedInt64Array);
	nat_setUint64(*natTen, 10);

	intOne = memnew(Ref<BigInt>);
	intOne->instantiate();
	(*intOne)->_set_abs(*natOne);

	floatThree = memnew(Ref<BigFloat>);
	floatThree->instantiate();
	(*floatThree)->SetUint64(3);

	cacheBase10 = memnew(Vector<BigDivisor>);
	// filled in lazily
}

void uninitialize_big_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	memdelete(natOne);
	natOne = nullptr;
	memdelete(natTwo);
	natTwo = nullptr;
	memdelete(natThree);
	natThree = nullptr;
	memdelete(natFive);
	natFive = nullptr;
	memdelete(natTen);
	natTen = nullptr;
	memdelete(intOne);
	intOne = nullptr;
	memdelete(floatThree);
	floatThree = nullptr;
	memdelete(cacheBase10);
	cacheBase10 = nullptr;
}

#ifdef GODOT_BIG_STANDALONE
extern "C" GDExtensionBool GDE_EXPORT big_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_big_module);
	init_obj.register_terminator(uninitialize_big_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_CORE);

	return init_obj.init();
}
#endif
