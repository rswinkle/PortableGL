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
		links { "SDL2", "m" }
	
	configuration "windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	configuration { "gmake" }
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }

	configuration { "gmake", "Release" }
	buildoptions { "-O3" }

	-- A project defines one build target

	project "zbuf_learn"
		files {
			"./zbuf_learn.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "line_perf"
		files {
			"./line_perf.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "point_perf"
		files {
			"./point_perf.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "clipping"
		files {
			"./test_clipping.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "edge_test"
		files {
			"./test_edges.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp"
		}

	project "line_test"
		files {
			"./test_lines.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "blend_test"
		files {
			"./blend_test.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "stencil_test"
		files {
			"./stencil_test.cpp",
			"../glcommon/rsw_math.cpp"
		}

	-- TODO this doesn't actually test all primitives...
	project "test_primitives"
		files {
			"./test_primitives.cpp",
			"../glcommon/rsw_math.cpp"
		}





