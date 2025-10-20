
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
workspace "Demos"
	configurations { "Debug", "Release" }
	-- stuff up here common to all projects
	kind "ConsoleApp"
	--location "build"
	--targetdir "build"
	targetdir "."

	s = os.capture("sdl2-config --cflags")

	--sdl_incdir = string.match(s, "-I(%g+)%s")
	sdl_incdir, sdl_def = string.match(s, "-I(%g+)%s+-D(%g+)")
	print(sdl_incdir, sdl_def)
	includedirs { "../", "../glcommon", "../external", sdl_incdir }
	libdirs { os.findlib("SDL2") }

	filter "system:linux"
		links { "SDL2", "m" }
	
	filter "system:windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	filter "Debug"
		defines { "DEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=long", sdl_def }
		-- symbols "On
		optimize "Debug"

	filter "Release"
		defines { "NDEBUG", "USING_PORTABLEGL", "CUTILS_SIZE_T=long", sdl_def }
		optimize "On"

	filter { "action:gmake", "language:C" }
		buildoptions { "-std=c99", "-pedantic-errors", "-Wall", "-Wextra", "-Wstrict-prototypes", "-Wno-unused-parameter", "-Wno-sign-compare" }

	filter { "action:gmake", "language:C++" }
		-- Stupid C++ warns about the standard = {0} initialization, but not the C++ only equivalent {} smh
		buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-missing-field-initializers", "-Wno-unused-parameter", "-Wno-sign-compare" }


	-- A project defines one build target
	project "swrenderer"
		language "C++"
		files {
			"./main.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "sphereworld"
		language "C++"
		files {
			"./sphereworld.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "sphereworld_color"
		language "C++"
		files {
			"./sphereworld_color.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "glm_sphereworld_color"
		language "C++"
		files {
			"./glm_sphereworld_color.cpp",
			"../glcommon/glm_glframe.cpp",
			"../glcommon/glm_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/glm_halfedge.cpp",
			"../glcommon/controls.cpp",
			"../glcommon/c_utils.cpp"
		}

	project "cubemap"
		language "C++"
		files {
			"./cubemap.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_primitives.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/rsw_glframe.cpp",
			"../glcommon/stb_image.h"
		}

	project "grass"
		language "C++"
		files {
			"./grass.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp"
		}

	project "gears"
		language "C"
		files {
			"./gears.c"
		}

	project "modelviewer"
		language "C"
		files {
			"./modelviewer.c",
			"../glcommon/chalfedge.c",
			"../glcommon/cprimitives.c"
		}

	project "pointsprites"
		language "C"
		files {
			"./pointsprites.c",
			"../glcommon/gltools.c",
			"../glcommon/gltools.h"
		}

	project "shadertoy"
		language "C++"
		--links { "SDL2", "m", "gomp" }
		files {
			"./shadertoy.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}

	project "raytracing_1weekend"
		language "C++"
		--links { "SDL2", "m", "gomp" }
		files {
			"./raytracing_1weekend.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}

	project "texturing"
		language "C++"
		files {
			"./texturing.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}

	project "texturing_ext"
		language "C++"
		files {
			"./texturing_ext.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}

	project "multidraw"
		language "C++"
		files {
			"./multidraw.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_matstack.h",
		}

--	project "polyline"
--		language "C++"
--		files {
--			"./polyline.cpp",
--			"../glcommon/rsw_math.cpp",
--			"../glcommon/rsw_matstack.h",
--		}

	project "testprimitives"
		language "C++"
		files {
			"./testprimitives.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/rsw_primitives.cpp",
		}

	project "particles"
		language "C++"
		files {
			"./particles.cpp",
			"../glcommon/rsw_math.cpp",
		}

	project "sdl_renderer_imgui"
		language "C++"
		includedirs { "./imgui", "./imgui/backends" }
		files {
			"./imgui/main.cpp",
			--"./imgui/imgui.cpp",
			--"./imgui/imgui_demo.cpp",
			--"./imgui/imgui_draw.cpp",
			--"./imgui/imgui_tables.cpp",
			--"./imgui/imgui_widgets.cpp",
			"./imgui/backends/imgui_impl_sdl.cpp",
			"./imgui/backends/imgui_impl_sdlrenderer.cpp"
		}

	project "pgl_imgui"
		language "C++"
		--linkoptions { "-fsanitize=address,undefined" }
		includedirs { "./imgui", "./imgui/backends" }
		files {
			"./imgui/main_pgl.cpp",
			"../glcommon/gltools.cpp",
			--"./imgui/imgui.cpp",
			--"./imgui/imgui_demo.cpp",
			--"./imgui/imgui_draw.cpp",
			--"./imgui/imgui_tables.cpp",
			--"./imgui/imgui_widgets.cpp",
			"./imgui/backends/imgui_impl_sdl.cpp",
			"./imgui/backends/imgui_impl_portablegl.cpp"
		}

	project "pgl_geometry_imgui"
		language "C++"
		--linkoptions { "-fsanitize=address,undefined" }
		includedirs { "./imgui", "./imgui/backends" }
		files {
			"./imgui/main_pgl_geometry.cpp",
			"../glcommon/gltools.cpp",
			--"./imgui/imgui.cpp",
			--"./imgui/imgui_demo.cpp",
			--"./imgui/imgui_draw.cpp",
			--"./imgui/imgui_tables.cpp",
			--"./imgui/imgui_widgets.cpp",
			"./imgui/backends/imgui_impl_sdl.cpp",
			"./imgui/backends/imgui_impl_pgl_geometry.cpp"
		}

	project "assimp_convert"
		language "C"
		links { "assimp", "m"}
		files {
			"./assimp_convert.c",
		}

