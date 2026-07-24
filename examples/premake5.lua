-- PortableGL examples
--
-- Generate build files from this directory:
--   Linux makefiles:     premake5 gmake
--   Linux CodeLite:      premake5 codelite
--   Windows VS 2022:     premake5 vs2022
--   Windows VS (cross):  premake5 vs2022 --os=windows   (from Linux/macOS)
--
-- Linux uses the system SDL2 (sdl2-config / pkg-config).
-- Windows uses the MSVC development files under ../external/SDL2
-- (see ../external/SDL2/README.md).
--
-- Projects live under original/, classic/, and webgl_lessons/ (via location).
-- Premake rewrites include/lib paths relative to those directories; post-build
-- DLL copy uses %{wks.location} so it stays correct from any project subdir.

function os.capture(cmd, raw)
	local f = io.popen(cmd, "r")
	if not f then
		return ""
	end
	local s = f:read("*a") or ""
	f:close()
	if raw then
		return s
	end
	s = string.gsub(s, "^%s+", "")
	s = string.gsub(s, "%s+$", "")
	s = string.gsub(s, "[\n\r]+", " ")
	return s
end

-- ---------------------------------------------------------------------------
-- SDL2 discovery
-- ---------------------------------------------------------------------------
local SDL2_ROOT = path.getabsolute("../external/SDL2")
local sdl_incdir = nil
local sdl_libdir = nil

if os.istarget("windows") then
	if not os.isdir(path.join(SDL2_ROOT, "include")) then
		print("WARNING: Windows SDL2 headers not found at " .. path.join(SDL2_ROOT, "include"))
		print("  Download SDL2-devel-*-VC.zip from https://github.com/libsdl-org/SDL/releases")
		print("  and extract so that external/SDL2/include/SDL.h exists.")
	else
		print("Using vendored SDL2 at " .. SDL2_ROOT)
	end
else
	local s = os.capture("sdl2-config --cflags --libs 2>/dev/null")
	if s == "" then
		s = os.capture("pkg-config --cflags --libs sdl2 2>/dev/null")
	end

	if s ~= "" then
		sdl_incdir = string.match(s, "-I(%S+)")
		sdl_libdir = string.match(s, "-L(%S+)")
	end

	if not sdl_libdir then
		sdl_libdir = os.findlib("SDL2")
	end

	if not sdl_incdir then
		sdl_incdir = "/usr/include/SDL2"
		print("WARNING: could not find SDL2 via sdl2-config/pkg-config; falling back to " .. sdl_incdir)
	else
		print("SDL2 include: " .. sdl_incdir .. (sdl_libdir and ("  lib: " .. sdl_libdir) or "  lib: (default search path)"))
	end
end

-- ---------------------------------------------------------------------------
-- Workspace
-- ---------------------------------------------------------------------------
workspace "Polished_Examples"
	configurations { "Debug", "Release" }
	architecture "x86_64"

	kind "ConsoleApp"
	targetdir "."

	-- Paths are relative to this script (examples/); Premake rewrites them for
	-- projects with location original/classic/webgl_lessons.
	includedirs { "../", "../glcommon", "../external" }

	filter "system:windows"
		systemversion "latest"
		includedirs { "../external/SDL2/include" }
		links { "SDL2main", "SDL2" }

	filter { "system:windows", "not action:vs*" }
		links { "mingw32" }

	-- %{wks.location} is the examples/ directory (where the .sln lives), so this
	-- path is correct whether the .vcxproj is in original/, classic/, or
	-- webgl_lessons/.
	filter { "system:windows", "architecture:x86_64" }
		libdirs { "../external/SDL2/lib/x64" }
		postbuildcommands {
			'{COPYFILE} "%{wks.location}../external/SDL2/lib/x64/SDL2.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "architecture:x86" }
		libdirs { "../external/SDL2/lib/x86" }
		postbuildcommands {
			'{COPYFILE} "%{wks.location}../external/SDL2/lib/x86/SDL2.dll" "%{cfg.targetdir}"'
		}

	filter "system:not windows"
		if sdl_incdir then
			includedirs { sdl_incdir }
		end
		if sdl_libdir then
			libdirs { sdl_libdir }
		end
		links { "SDL2" }

	filter "system:linux"
		links { "m" }

	-- Dialects apply to all generators (gmake, VS, etc.)
	filter "language:C"
		cdialect "C99"

	filter "language:C++"
		cppdialect "C++20"

	filter { "action:gmake*", "language:C" }
		buildoptions {
			"-pedantic-errors",
			"-Wall",
			"-Wextra",
			"-Wstrict-prototypes",
			"-Wno-unused-parameter",
			"-Wno-unknown-pragmas",
		}

	filter { "action:gmake*", "language:C++" }
		-- C++ warns about = {0} but not the C++-only {} equivalent
		buildoptions {
			"-fno-rtti",
			"-fno-exceptions",
			"-fno-strict-aliasing",
			"-Wall",
			"-Wextra",
			"-Wno-missing-field-initializers",
			"-Wno-unused-parameter",
			"-Wno-unknown-pragmas",
		}

	filter "configurations:Debug"
		defines { "DEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=int" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=int" }
		optimize "On"

	filter {}

	-- -------------------------------------------------------------------
	-- Original / custom
	-- -------------------------------------------------------------------
	project "c_ex1"
		targetdir "original"
		location "original"
		language "C"
		files {
			"original/ex1.c",
		}

	project "std_shader_ex1"
		targetdir "original"
		location "original"
		language "C"
		files {
			"original/ex1_std_shaders.c",
		}

	project "c_ex2"
		targetdir "original"
		location "original"
		language "C"
		files {
			"original/ex2.c",
		}

	project "std_shader_ex2"
		targetdir "original"
		location "original"
		language "C"
		files {
			"original/ex2_std_shaders.c",
		}

	project "c_ex3"
		targetdir "original"
		location "original"
		language "C"
		files {
			"original/ex3.c",
		}

	project "ex1"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"original/ex1.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "ex2"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"original/ex2.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "ex3"
		targetdir "original"
		location "original"
		language "C++"
		files {
			"original/ex3.cpp",
			"../glcommon/rsw_math.cpp",
		}

	-- -------------------------------------------------------------------
	-- Classic OpenGL ports
	-- -------------------------------------------------------------------
	project "gears"
		targetdir "classic"
		location "classic"
		language "C"
		files {
			"classic/gears.c",
		}

	-- -------------------------------------------------------------------
	-- WebGL lessons
	-- -------------------------------------------------------------------
	project "lesson1"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson1.cpp",
		}

	project "lesson2"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson2.cpp",
		}

	project "lesson3"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson3.cpp",
		}

	project "lesson4"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson4.cpp",
		}

	project "lesson5"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson5.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson6"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson6.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson7"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson7.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson8"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson8.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson9"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson9.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson10"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson10.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "lesson11"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson11.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "lesson12"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson12.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson13"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson13.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson14"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson14.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson15"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson15.cpp",
			"../glcommon/gltools.cpp",
		}

	project "lesson16"
		targetdir "webgl_lessons"
		location "webgl_lessons"
		language "C++"
		files {
			"webgl_lessons/lesson16.cpp",
			"../glcommon/gltools.cpp",
		}
