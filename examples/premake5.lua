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

workspace "Polished_Examples"
	configurations { "Debug", "Release" }
	kind "ConsoleApp"
	targetdir "."

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


	filter "system:linux"
		links { "SDL2", "m" }

	filter "system:windows"
		--libdirs "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	filter "Debug"
		defines { "DEBUG", "USING_PORTABLEGL", sdl_def }
		--symbols "On"
		optimize "Debug"

	filter "Release"
		defines { "NDEBUG", "USING_PORTABLEGL", sdl_def }
		optimize "On"

	filter { "action:gmake", "Release" }
		buildoptions { "-O3" }

	filter { "action:gmake", "language:C" }
		buildoptions { "-std=c99", "-pedantic-errors", "-Wall", "-Wextra", "-Wstrict-prototypes" }
	filter { "action:gmake", "language:C++" }
		-- Stupid C++ warns about the standard = {0} initialization, but not the C++ only equivalent {} smh
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-missing-field-initializers" }

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

	-- does this work
	filter "system:linux"
		links { "SDL2", "m" }

	filter "system:windows"
		--libdirs "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32" }

	project "minimal_pgl"
		includedirs { "../" }
		language "C"
		files {
			"./minimal_pgl.c"
		}

