-- A solution contains projects, and defines the available configurations
solution "GLCommon"
   configurations { "Debug", "Release" }
   location "tests/build"
 
   -- A project defines one build target
   project "test_crsw_math"
      kind "ConsoleApp"
      language "C"
      files {
      	  "tests/main.c",
      	  "crsw_math.h"
      }
      excludes
      {
      --  "vector_template*", "cvector.h", "vector_tests2.c"
      }
      includedirs { "./" }
--      libdirs { }
--      links { "SDL2", "GLEW", "GL" } 
	  targetdir "tests/build"
  
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
 
      configuration { "linux", "gmake" }
         buildoptions { "-ansi", "-fno-strict-aliasing", "-Wunused-variable", "-Wreturn-type" }


