
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

	filter { "gmake", "Release" }
		buildoptions { "-O3" }

	project "c_ex1"
		language "C"
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex1.c"
		}

	project "c_ex2"
		language "C"
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex2.c"
		}

	project "c_ex3"
		language "C"
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex3.c"
		}

	project "ex1"
		language "C++"
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex1.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex2"
		language "C++"
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex2.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex3"
		language "C++"
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }
		files {
			"./ex3.cpp",
			"../glcommon/rsw_math.cpp"
		}

