-- A solution contains projects, and defines the available configurations
solution "Demos"
	configurations { "Debug", "Release" }
	
	includedirs { "../", "../glcommon", "/usr/include/SDL2" }

	-- stuff up here common to all projects
	kind "ConsoleApp"
	--location "build"
	--targetdir "build"
	targetdir "."

	configuration "linux"
		links { "SDL2", "m" }
	
	configuration "windows"
		--linkdir "/mingw64/lib"
		--buildoptions "-mwindows"
		links { "mingw32", "SDL2main", "SDL2" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }

	configuration { "gmake", "Release" }
	buildoptions { "-O3" }

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
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

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
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

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
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

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
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	project "grass"
		language "C++"
		files {
			"./grass.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_glframe.cpp"
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }

	project "gears"
		language "C"
		files {
			"./gears.c"
		}
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }

	project "modelviewer"
		language "C"
		files {
			"./modelviewer.c",
			"../glcommon/chalfedge.c",
			"../glcommon/cprimitives.c"
		}
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }

	project "pointsprites"
		language "C"
		files {
			"./pointsprites.c",
			"../glcommon/gltools.c",
			"../glcommon/gltools.h"
		}
		configuration { "gmake" }
			buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }

	project "shadertoy"
		language "C++"
		files {
			"./shadertoy.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type", "-fopenmp" }
			links { "SDL2", "m", "gomp" }

	project "raytracing_1weekend"
		language "C++"
		files {
			"./raytracing_1weekend.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type", "-fopenmp" }
			links { "SDL2", "m", "gomp" }

	project "texturing"
		language "C++"
		files {
			"./texturing.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/gltools.cpp",
			"../glcommon/stb_image.h"
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type", "-fopenmp" }
			links { "SDL2", "m", "gomp" }

	project "multidraw"
		language "C++"
		files {
			"./multidraw.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_matstack.h",
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }
			links { "SDL2", "m" }

	project "testprimitives"
		language "C++"
		files {
			"./testprimitives.cpp",
			"../glcommon/rsw_math.cpp",
			"../glcommon/rsw_halfedge.cpp",
			"../glcommon/rsw_primitives.cpp",
		}
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }
			links { "SDL2", "m" }

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
		configuration { "gmake" }
			buildoptions { "-fno-rtti", "-fno-exceptions", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type", "-fopenmp" }
			links { "SDL2", "m", "gomp" }


	project "assimp_convert"
		language "C"
		files {
			"./assimp_convert.c",
		}

		links { "assimp", "m"}

		configuration { "gmake" }
		buildoptions { "-std=c99", "-pedantic-errors", "-Wunused-variable", "-Wreturn-type" }

		configuration { "gmake", "Release" }
		buildoptions { "-O3" }
