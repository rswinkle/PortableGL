-- PortableGL demos
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
-- Extra system deps (not vendored):
--   video_texturing  — FFmpeg libav* (large; not practical to vendor).
--                      Intended for Linux/pkg installs. Will not link on
--                      Windows unless the user installs FFmpeg dev libs and
--                      points the project at them themselves.
--   assimp_convert   — Assimp (headers ~1MB, one .so/.dll ~7–15MB; Windows
--                      x64 release zip ~18MB — small enough to vendor later
--                      if we want a zero-setup Windows build for this tool).

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
workspace "Demos"
	configurations { "Debug", "Release" }
	architecture "x86_64"

	kind "ConsoleApp"
	language "C++"
	targetdir "."

	includedirs { "../", "../glcommon", "../external" }

	-- Most demos need SDL2; applied workspace-wide (assimp_convert also gets the
	-- link, which is harmless).
	filter "system:windows"
		systemversion "latest"
		includedirs { "../external/SDL2/include" }
		links { "SDL2main", "SDL2" }

	filter { "system:windows", "not action:vs*" }
		links { "mingw32" }

	filter { "system:windows", "architecture:x86_64" }
		libdirs { "../external/SDL2/lib/x64" }
		postbuildcommands {
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

	filter "system:linux"
		links { "m" }

	-- Dialects apply to all generators (gmake, VS, etc.)
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
	filter { "action:gmake*", "language:C" }
		buildoptions {
			"-pedantic-errors",
			"-Wall",
			"-Wextra",
			"-Wstrict-prototypes",
			"-Wno-unused-parameter",
			"-Wno-sign-compare",
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
			"-Wno-sign-compare",
		}

	filter "configurations:Debug"
		defines { "DEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=long" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=long" }
		optimize "On"

	filter {}

	-- -------------------------------------------------------------------
	project "swrenderer"
		language "C++"
		files {
			"main.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "sphereworld"
		language "C++"
		files {
			"sphereworld.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "sphereworld_color"
		language "C++"
		files {
			"sphereworld_color.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "glm_sphereworld_color"
		language "C++"
		files {
			"glm_sphereworld_color.cpp",
			"../glcommon/glm_glframe.cpp",
			"../glcommon/glm_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/glm_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp",
		}

	project "cubemap"
		language "C++"
		files {
			"cubemap.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/stb_image.h",
		}

	project "grass"
		language "C++"
		files {
			"grass.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
		}

	project "modelviewer"
		language "C"
		files {
			"modelviewer.c",
			"../glcommon/chalfedge.c",
			"../glcommon/cprimitives.c",
		}

	project "pointsprites"
		language "C"
		files {
			"pointsprites.c",
			"../glcommon/gltools.c",
			"../glcommon/gltools.h",
		}

	project "shadertoy"
		language "C++"
		files {
			"shadertoy.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h",
		}

	project "raytracing_1weekend"
		language "C++"
		files {
			"raytracing_1weekend.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h",
		}

	project "texturing"
		language "C++"
		files {
			"texturing.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h",
		}

	project "texturing_ext"
		language "C++"
		files {
			"texturing_ext.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h",
		}

	project "video_texturing"
		language "C++"
		-- FFmpeg (libavformat/avcodec/swscale/avutil) is a system dependency and
		-- is NOT vendored — full Windows builds of those libs are huge.
		-- On Linux: install distro packages (e.g. libavformat-dev etc.) and this
		-- target builds like the other demos.
		-- On Windows / MSVC: this project is left in the solution for completeness
		-- but will not build out of the box. To build it yourself, install FFmpeg
		-- development libraries, then add the include/lib paths (and DLLs) for
		-- avformat, avcodec, swscale, and avutil to this project.
		links { "avformat", "avcodec", "swscale", "avutil" }
		files {
			"video_texturing.cpp",
			"video_texture.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h",
		}

	project "multidraw"
		language "C++"
		files {
			"multidraw.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_matstack.h",
		}

--	project "polyline"
--		language "C++"
--		files {
--			"polyline.cpp",
--			"../glcommon/rsw_math.cpp",
--			"../glcommon/rsw_matstack.h",
--		}

	project "testprimitives"
		language "C++"
		files {
			"testprimitives.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/rsw_primitives.cpp",
		}

	project "particles"
		language "C++"
		files {
			"particles.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "sdl_renderer_imgui"
		language "C++"
		includedirs { "../external/imgui", "../external/imgui/backends" }
		files {
			"imgui/main.cpp",
			"../external/imgui/backends/imgui_impl_sdl2.cpp",
			"../external/imgui/backends/imgui_impl_sdlrenderer2.cpp",
		}

	project "pgl_imgui"
		language "C++"
		includedirs { "../external/imgui", "../external/imgui/backends" }
		files {
			"imgui/main_pgl.cpp",
			"../glcommon/gltools.cpp",
			"../external/imgui/backends/imgui_impl_sdl2.cpp",
			"../external/imgui/backends/imgui_impl_portablegl.cpp",
		}

	project "pgl_geometry_imgui"
		language "C++"
		includedirs { "../external/imgui", "../external/imgui/backends" }
		files {
			"imgui/main_pgl_geometry.cpp",
			"../glcommon/gltools.cpp",
			"../external/imgui/backends/imgui_impl_sdl2.cpp",
			"../external/imgui/backends/imgui_impl_pgl_geometry.cpp",
		}

	project "assimp_convert"
		language "C"
		-- Offline model converter; needs Assimp (system package on Linux).
		-- Assimp is small enough that we could vendor Windows binaries later
		-- (headers ~1MB + one DLL/import lib ~10MB, vs multi‑tens of MB for FFmpeg).
		links { "assimp" }
		files {
			"assimp_convert.c",
		}
