
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
	
	s = os.capture("sdl2-config --cflags --libs")

	--sdl_incdir = string.match(s, "-I(%g+)%s")
	sdl_incdir, sdl_def, sdl_libdir = string.match(s, "-I(%g+)%s+-D(%g+)%s+-L(%g+)")
	if not sdl_incdir then
		sdl_incdir, sdl_def = string.match(s, "-I(%g+)%s+-D(%g+)")
		--not really necessary since if it should be in a standard search path if
		--sdl2-config didn't specify a -L
		sdl_libdir = os.findlib("SDL2")
	end
	print(sdl_incdir, sdl_def, sdl_libdir)
	includedirs { "../", "../glcommon", "../external", sdl_incdir }
	libdirs { sdl_libdir }

	-- stuff up here common to all projects
	kind "ConsoleApp"
	language "C++"
	--location "build"
	--targetdir "build"
	targetdir "."

	filter "system:linux"
		links { "m" }
	
	filter "system:windows"
		--libdirs "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main" }

	filter { "action:gmake" }
		buildoptions { "-ffp-contract=off", "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	filter "Debug"
		defines { "DEBUG", "USING_PORTABLEGL", sdl_def }
		symbols "On"
		--optimize "Debug"

	filter "Release"
		defines { "NDEBUG", "USING_PORTABLEGL", sdl_def }
		optimize "On"

	filter { "action:gmake", "Debug" }
		buildoptions { "-fsanitize=address,undefined" }
		linkoptions { "-fsanitize=address,undefined" }

	-- A project defines one build target
	project "perf_tests"
		includedirs { "../", "../glcommon", sdl_incdir }
		libdirs { os.findlib("SDL2") }
		links { "SDL2" }
		files {
			"./performance_tests.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "skybox_clipping"
		includedirs { "../", "../glcommon", sdl_incdir }
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

	project "line_testing"
		language "C"
		includedirs { "../", "../glcommon", sdl_incdir }
		libdirs { os.findlib("SDL2") }
		links { "SDL2" }
		files {
			"./lines.c"
		}


	project "math_testing"
		includedirs { "../", "../glcommon", "../external/glm" }
		files {
			"./math_testing.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "minimal_pgl"
		includedirs { "../" }
		language "C"
		files {
			"./minimal_pgl.c"
		}

	project "run_tests"
		includedirs { "../", "../glcommon" }
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	-- use defines to run the same tests with 16 bit pixel formats
	-- and 16 bit zbuf etc.
	project "run_tests_rgb565"
		includedirs { "../", "../glcommon" }
		defines { "PGL_RGB565" }
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	project "run_tests_d16"
		includedirs { "../", "../glcommon" }
		defines { "PGL_D16" }
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	project "run_tests_d16_no_stencil"
		includedirs { "../", "../glcommon" }
		defines { "PGL_D16", "PGL_NO_STENCIL" }
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

	project "run_tests_clamp_border"
		includedirs { "../", "../glcommon" }
		defines { "PGL_ENABLE_CLAMP_TO_BORDER" }
		files {
			"./run_tests.cpp",
			"../glcommon/gltools.cpp"
		}

