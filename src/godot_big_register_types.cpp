#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

#include "godot_big_register_types.h"
#include "godot_big_int.h"
#include "godot_big_rat.h"
#include "godot_big_float.h"

using namespace godot;

Ref<BigInt> *intOne = nullptr;
Ref<BigFloat> *floatThree = nullptr;
Vector<BigDivisor> *cacheBase10 = nullptr;
std::mutex cacheBase10_mutex;

void initialize_big_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	GDREGISTER_CLASS(BigInt);
	GDREGISTER_CLASS(BigRat);
	GDREGISTER_CLASS(BigFloat);

	intOne = memnew(Ref<BigInt>);
	*intOne = BigInt::NewInt(1);
	floatThree = memnew(Ref<BigFloat>);
	*floatThree = BigFloat::NewFloat(3.0);
	cacheBase10 = memnew(Vector<BigDivisor>);
}

void uninitialize_big_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

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
