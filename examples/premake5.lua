
workspace "Polished_Examples"
	configurations { "Debug", "Release" }
	
	targetdir "."
	includedirs { "../", "../glcommon", "/usr/include/SDL2" }

	kind "ConsoleApp"

	filter "system:linux"
		links { "SDL2", "m" }

	filter "system:windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	filter "Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "Release"
		defines { "NDEBUG" }
		optimize "On"

	filter { "action:gmake", "Release" }
		buildoptions { "-O3" }

	filter { "action:gmake", "language:C" }
		buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }
	filter { "action:gmake", "language:C++" }
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	project "c_ex1"
		language "C"
		files {
			"./ex1.c"
		}

	project "std_shader_ex1"
		language "C"
		files {
			"./ex1_std_shaders.c"
		}

	project "line_testing"
		language "C"
		files {
			"./lines.c"
		}

	project "c_ex2"
		language "C"
		files {
			"./ex2.c"
		}

	project "std_shader_ex2"
		language "C"
		files {
			"./ex2_std_shaders.c"
		}

	project "c_ex3"
		language "C"
		files {
			"./ex3.c"
		}

	project "ex1"
		language "C++"
		files {
			"./ex1.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex2"
		language "C++"
		files {
			"./ex2.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex3"
		language "C++"
		files {
			"./ex3.cpp",
			"../glcommon/rsw_math.cpp"
		}

