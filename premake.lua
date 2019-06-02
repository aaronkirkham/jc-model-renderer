workspace "jc-model-renderer"
  configurations { "Debug", "Release" }
  location "out"
  systemversion "latest"
  language "C++"
  targetdir "out/%{cfg.buildcfg}"
  objdir "out"
  cppdialect "c++17"
  characterset "MBCS"
  architecture "x64"
  disablewarnings { "26451", "26491", "26495", "28020" }
  defines { "WIN32", "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" }

  filter "configurations:Debug"
    defines { "DEBUG", "_DEBUG", "_ITERATOR_DEBUG_LEVEL=0" }
    symbols "On"

  filter "configurations:Release"
    optimize "On"

-- Application
project "jc-model-renderer"
  kind "WindowedApp"
  defines "CPPHTTPLIB_ZLIB_SUPPORT"
  dependson { "imgui", "zlib", "tinyxml2" }
  links { "Advapi32", "Comdlg32", "d3d11", "D3DCompiler", "dxgi", "Gdi32", "Ole32", "Shell32", "ws2_32", "out/%{cfg.buildcfg}/imgui", "out/%{cfg.buildcfg}/zlib", "out/%{cfg.buildcfg}/tinyxml2" }
  files { "src/assets.rc", "src/**.h", "src/**.cpp" }

  includedirs {
    "src",
    "vendor/glm",
    "vendor/httplib",
    "vendor/imgui",
    "vendor/json/single_include/nlohmann",
    "vendor/ksignals",
    "vendor/zlib",
    "vendor/spdlog/include",
    "vendor/tinyxml2"
  }

  disablewarnings { "4200", "4244", "4267", "6031", "6262" }

-- ImGui
project "imgui"
  kind "StaticLib"
  defines "IMGUI_DISABLE_OBSOLETE_FUNCTIONS"
  files {
    "vendor/imgui/*.h",
    "vendor/imgui/*.cpp",
    "vendor/imgui/examples/imgui_impl_win32.cpp",
    "vendor/imgui/examples/imgui_impl_win32.h",
    "vendor/imgui/examples/imgui_impl_dx11.cpp",
    "vendor/imgui/examples/imgui_impl_dx11.h",
    "vendor/imgui/misc/cpp/imgui_stdlib.cpp",
    "vendor/imgui/misc/cpp/imgui_stdlib.h"
  }
  removefiles { "vendor/imgui/imgui_demo.cpp" }

  includedirs {
    "vendor/imgui",
    "vendor/imgui/examples",
    "vendor/imgui/misc/cpp"
  }

-- zlib
project "zlib"
  kind "StaticLib"
  defines "DEF_WBITS=-15"
  files { "vendor/zlib/*.c", "vendor/zlib/*.h" }
  includedirs { "vendor/zlib" }
  disablewarnings { "4996", "4267" }

-- tinyxml2
project "tinyxml2"
kind "StaticLib"
files { "vendor/tinyxml2/*.cpp", "vendor/tinyxml2/*.h" }
includedirs { "vendor/tinyxml2" }
-- disablewarnings { "4996", "4267" }
