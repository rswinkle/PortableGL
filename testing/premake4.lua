
function os.capture(cmd, raw)
  local f = assert(io.popen(cmd, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  if raw then return s end
  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  s = string.gsub(s, '[\n\r]+', ' ')
  return s
end


-- A solution contains projects, and defines the available configurations
solution "Testing"
	configurations { "Debug", "Release" }
	
	s = os.capture("sdl2-config --cflags")

	-- premake4 uses Lua 5.1 which doesn't have %g
	--sdl_incdir = string.match(s, "-I(%g+)%s")
	sdl_incdir, sdl_def = string.match(s, "-I([^%s]+)%s+-D([^%s]+)")
	print(sdl_incdir, sdl_def)
	includedirs { "../", "../glcommon", sdl_incdir }

	-- stuff up here common to all projects
	kind "ConsoleApp"
	language "C++"
	--location "build"
	--targetdir "build"
	targetdir "."

	configuration "linux"
		links { "m" }
	
	configuration "windows"
		--libdirs "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main" }

	configuration { "gmake" }
		buildoptions { "-ffp-contract=off", "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	configuration "Debug"
		defines { "DEBUG", "USING_PORTABLEGL", sdl_def }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG", "USING_PORTABLEGL", sdl_def }
		flags { "Optimize" }

	configuration { "gmake", "Release" }
	buildoptions { "-O3" }

	configuration { "gmake", "Debug" }
		buildoptions { "-fsanitize=address,undefined" }
		linkoptions { "-fsanitize=address,undefined" }

	-- A project defines one build target
	project "run_tests"
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	project "perf_tests"
		libdirs { os.findlib("SDL2") }
		links { "SDL2" }
		files {
			"./performance_tests.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "skybox_clipping"
		libdirs { os.findlib("SDL2") }
		links { "SDL2" }
		files {
			"./skybox_clipping.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/stb_image.h"

		}

	project "math_testing"
		libdirs { os.findlib("SDL2") }
		links { "SDL2" }
	  includedirs { "../external/glm" }
		files {
			"./math_testing.cpp",
			"../glcommon/rsw_math.cpp"
		}


