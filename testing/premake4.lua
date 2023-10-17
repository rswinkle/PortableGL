-- A solution contains projects, and defines the available configurations
solution "Testing"
	configurations { "Debug", "Release" }
	
	includedirs { "../", "../glcommon", "/usr/include/SDL2" }

	-- stuff up here common to all projects
	kind "ConsoleApp"
	language "C++"
	--location "build"
	--targetdir "build"
	targetdir "."

	configuration "linux"
		links { "SDL2", "m" }
	
	configuration "windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	configuration { "gmake" }
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	configuration "Debug"
		defines { "DEBUG", "USING_PORTABLEGL" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG", "USING_PORTABLEGL" }
		flags { "Optimize" }

	configuration { "gmake", "Release" }
	buildoptions { "-O3" }

	configuration { "gmake", "Debug" }
		buildoptions { "-fsanitize=address" }
		linkoptions { "-fsanitize=address" }

	-- A project defines one build target
	project "run_tests"
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	project "perf_tests"
		files {
			"./performance_tests.cpp",
			"../glcommon/rsw_math.cpp"
		}

