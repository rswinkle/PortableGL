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
		symbols "On"
		--optimize "Debug"

	filter "Release"
		defines { "NDEBUG", "USING_PORTABLEGL", sdl_def }
		optimize "On"

	filter { "action:gmake", "language:C" }
		cdialect "C99"
		buildoptions { "-pedantic-errors", "-Wall", "-Wextra", "-Wstrict-prototypes", "-Wno-unused-parameter", "-Wno-unknown-pragmas" }
	filter { "action:gmake", "language:C++" }
		cppdialect "C++20"
		-- Stupid C++ warns about the standard = {0} initialization, but not the C++ only equivalent {} smh
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-missing-field-initializers", "-Wno-unused-parameter", "-Wno-unknown-pragmas" }

-- Original/custom
--
	project "c_ex1"
		targetdir "original"
		location "original"
		language "C"
		files {
			"./original/ex1.c"
		}

	project "std_shader_ex1"
		targetdir "original"
		location "original"
		language "C"
		files {
			"./original/ex1_std_shaders.c"
		}

	project "c_ex2"
		targetdir "original"
		location "original"
		language "C"
		files {
			"./original/ex2.c"
		}

	project "std_shader_ex2"
		targetdir "original"
		location "original"
		language "C"
		files {
			"./original/ex2_std_shaders.c"
		}

	project "c_ex3"
		targetdir "original"
		location "original"
		language "C"
		files {
			"./original/ex3.c"
		}

	project "ex1"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"./original/ex1.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex2"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"./original/ex2.cpp",
			"../glcommon/rsw_math.cpp"
		}

	project "ex3"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"./original/ex3.cpp",
			"../glcommon/rsw_math.cpp"
		}



-- Misc OpenGL Ports
	project "gears"
		targetdir "classic"
		location "classic"
		language "C"
		files {
			"./classic/gears.c"
		}


-- WebGL lessons
	project "lesson1"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson1.cpp"
		}

	project "lesson2"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson2.cpp"
		}

	project "lesson3"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson3.cpp"
		}

	project "lesson4"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson4.cpp"
		}

	project "lesson5"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson5.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson6"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson6.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson7"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson7.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson8"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson8.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson9"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson9.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson10"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson10.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "lesson11"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson11.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "lesson12"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson12.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson13"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson13.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson14"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson14.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson15"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson15.cpp",
			"../glcommon/gltools.cpp"
		}

	project "lesson16"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"./webgl_lessons/lesson16.cpp",
			"../glcommon/gltools.cpp"
		}
