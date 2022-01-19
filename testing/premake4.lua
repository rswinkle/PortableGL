-- A solution contains projects, and defines the available configurations
solution "Testing"
	configurations { "Debug", "Release" }
	
	includedirs { "../", "../glcommon", "/usr/local/include" }

	-- stuff up here common to all projects
	kind "ConsoleApp"
	language "C++"
	--location "build"
	--targetdir "build"
	targetdir "."

	configuration "linux"
		links { "SDL2", "m", "gomp" }
	
	configuration "windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	configuration { "gmake" }
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type", "-fopenmp" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
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

