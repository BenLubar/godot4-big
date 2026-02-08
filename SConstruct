#!/usr/bin/env python

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPDEFINES=["GODOT_BIG_STANDALONE"])

sources = SConscript("./SCsub", {"env": env})

if env["target"] in ["editor", "template_debug"]:
	doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
	sources.append(doc_data)

if env["platform"] == "macos":
	library = env.SharedLibrary(
		"project/bin/libgdbig.{}.{}.framework/libgdbig.{}.{}".format(
			env["platform"], env["target"], env["platform"], env["target"]
		),
		source=sources,
	)
elif env["platform"] == "ios":
	if env["ios_simulator"]:
		library = env.StaticLibrary(
			"project/bin/libgdbig.{}.{}.simulator.a".format(env["platform"], env["target"]),
			source=sources,
		)
	else:
		library = env.StaticLibrary(
			"project/bin/libgdbig.{}.{}.a".format(env["platform"], env["target"]),
			source=sources,
		)
else:
	library = env.SharedLibrary(
		"project/bin/libgdbig{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
		source=sources,
	)

env.NoCache(library)
Default(library)
