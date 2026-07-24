-- PortableGL testing suite
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

	-- sdl2-config often omits -L when SDL is in a default linker path
	if not sdl_libdir then
		sdl_libdir = os.findlib("SDL2")
	end

	if not sdl_incdir then
		-- Common fallback; keep generation usable even if discovery fails
		sdl_incdir = "/usr/include/SDL2"
		print("WARNING: could not find SDL2 via sdl2-config/pkg-config; falling back to " .. sdl_incdir)
	else
		print("SDL2 include: " .. sdl_incdir .. (sdl_libdir and ("  lib: " .. sdl_libdir) or "  lib: (default search path)"))
	end
end

-- Apply SDL2 include/lib/link settings to the current project.
-- Call inside each project that needs SDL2, then filter {} is left clear.
local function use_sdl2()
	filter "system:windows"
		includedirs { "../external/SDL2/include" }
		-- SDL2main provides WinMain; safe even when sources define SDL_MAIN_HANDLED
		links { "SDL2main", "SDL2" }

	-- MinGW (gmake / non-VS IDEs on Windows): mingw32 must precede SDL2main
	filter { "system:windows", "not action:vs*" }
		links { "mingw32" }

	-- Use paths relative to the project files (testing/) so VS projects generated
	-- on Linux with --os=windows still work when opened on Windows.
	filter { "system:windows", "architecture:x86_64" }
		libdirs { "../external/SDL2/lib/x64" }
		postbuildcommands {
			-- Quotes required: unquoted "." as targetdir drops the destination path
			'{COPYFILE} "../external/SDL2/lib/x64/SDL2.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "architecture:x86" }
		libdirs { "../external/SDL2/lib/x86" }
		postbuildcommands {
			'{COPYFILE} "../external/SDL2/lib/x86/SDL2.dll" "%{cfg.targetdir}"'
		}

	filter "system:not windows"
		if sdl_incdir then
			includedirs { sdl_incdir }
		end
		if sdl_libdir then
			libdirs { sdl_libdir }
		end
		links { "SDL2" }

	filter {}
end

-- ---------------------------------------------------------------------------
-- Workspace
-- ---------------------------------------------------------------------------
workspace "Testing"
	configurations { "Debug", "Release" }
	-- Default to 64-bit; override with e.g. premake5 --arch=x86 vs2022
	architecture "x86_64"

	kind "ConsoleApp"
	language "C++"
	targetdir "."

	includedirs { "../", "../glcommon", "../external" }

	filter "system:linux"
		links { "m" }

	filter "system:windows"
		-- Avoid forcing a specific Windows SDK; "latest" works with current VS installs
		systemversion "latest"

	-- Dialects apply to all generators (gmake, VS, etc.). MSVC needs C++20 for
	-- designated initializers used in tests (e.g. stencil.cpp).
	filter "language:C"
		cdialect "C99"

	filter "language:C++"
		cppdialect "C++20"

	-- MSVC: PGL headers generate huge numbers of conversion warnings (double↔float,
	-- size_t↔int, etc.) that drown out real issues. Keep useful warnings, silence noise.
	filter { "action:vs*" }
		disablewarnings {
			"4244", -- conversion, possible loss of data (e.g. double→float)
			"4305", -- truncation from 'double' to 'float'
			"4267", -- conversion from 'size_t' to smaller type
			"4018", -- signed/unsigned mismatch
			"4100", -- unreferenced formal parameter
			"4101", -- unreferenced local variable
			"4189", -- local variable initialized but not referenced
			"4127", -- conditional expression is constant
			"4201", -- nonstandard extension: nameless struct/union
			"4456", -- declaration hides previous local
			"4457", -- declaration hides function parameter
			"4458", -- declaration hides class member
			"4505", -- unreferenced local function removed
			"4701", -- potentially uninitialized local variable
			"4702", -- unreachable code
			"4996", -- deprecated CRT / POSIX names
		}

	-- GCC/Clang flags (makefiles and non-VS IDEs). Do not apply to MSVC.
	filter { "action:gmake*" }
		buildoptions {
			"-ffp-contract=off",
			"-fno-strict-aliasing",
			"-Wunused-variable",
			"-Wreturn-type",
		}

	filter { "action:gmake*", "language:C++" }
		buildoptions { "-fno-rtti", "-fno-exceptions" }

	filter { "action:gmake*", "configurations:Debug" }
		buildoptions { "-fsanitize=address,undefined" }
		linkoptions { "-fsanitize=address,undefined" }

	filter "configurations:Debug"
		defines { "DEBUG", "USING_PORTABLEGL" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG", "USING_PORTABLEGL" }
		optimize "On"

	filter {}

	-- -------------------------------------------------------------------
	-- Projects that need SDL2 (windowed / interactive)
	-- -------------------------------------------------------------------
	project "perf_tests"
		use_sdl2()
		files {
			"performance_tests.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
		}

	project "perf_tests_small_tex"
		use_sdl2()
		defines { 'TEX_PATH="../media/textures/star.gif"' }
		files {
			"performance_tests.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
		}

	project "skybox_clipping"
		use_sdl2()
		files {
			"skybox_clipping.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/stb_image.h",
		}

	project "line_testing"
		language "C"
		use_sdl2()
		files {
			"lines.c",
		}

	-- -------------------------------------------------------------------
	-- Headless / no SDL
	-- -------------------------------------------------------------------
	project "math_testing"
		includedirs { "../external/glm" }
		files {
			"math_testing.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "minimal_pgl"
		language "C"
		files {
			"minimal_pgl.c",
		}

	project "run_tests"
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}

	-- Same tests with alternate pixel / buffer configurations
	project "run_tests_rgb565"
		defines { "PGL_RGB565" }
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}

	project "run_tests_d16"
		defines { "PGL_D16" }
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}

	project "run_tests_d16_no_stencil"
		defines { "PGL_D16", "PGL_NO_STENCIL" }
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}

	project "run_tests_clamp_border"
		defines { "PGL_ENABLE_CLAMP_TO_BORDER" }
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}

	project "run_tests_no_depth_no_stencil"
		defines { "PGL_NO_DEPTH_NO_STENCIL" }
		files {
			"run_tests.cpp",
			"../glcommon/gltools.cpp",
		}
