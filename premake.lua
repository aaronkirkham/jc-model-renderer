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

project "jc-model-renderer"
  kind "WindowedApp"
  defines "CPPHTTPLIB_ZLIB_SUPPORT"
  dependson { "imgui", "zlib", "tinyxml2", "AvaFormatLib" }
  links {
    "Advapi32",
    "Comdlg32",
    "d3d11",
    "D3DCompiler",
    "dxgi",
    "Gdi32",
    "Ole32",
    "Shell32",
    "ws2_32",
    "imgui",
    "tinyxml2",
    "AvaFormatLib"
  }
  files { "src/assets.rc", "src/**.h", "src/**.cpp" }

  includedirs {
    "src",
    "vendor/argparse",
    "vendor/glm",
    "vendor/httplib",
    "vendor/imgui",
    "vendor/rapidjson/include",
    "vendor/ksignals/include/ksignals",
    "vendor/spdlog/include",
    "vendor/tinyxml2",
    "vendor/ava-format-lib/include",
    "vendor/ava-format-lib/deps/zlib"
  }

  disablewarnings { "4200", "4244", "4267", "6031", "6262" }

  filter "configurations:Debug"
    defines "SPDLOG_ACTIVE_LEVEL=1"

group "vendor"
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
    -- removefiles { "vendor/imgui/imgui_demo.cpp" }

    includedirs {
      "vendor/imgui",
      "vendor/imgui/examples",
      "vendor/imgui/misc/cpp"
    }

  project "tinyxml2"
    kind "StaticLib"
    files { "vendor/tinyxml2/tinyxml2.cpp", "vendor/tinyxml2/tinyxml2.h" }
    includedirs { "vendor/tinyxml2" }

  project "AvaFormatLib"
    kind "StaticLib"
    includedirs "vendor/ava-format-lib/deps"
    files { "vendor/ava-format-lib/src/**.cpp", "vendor/ava-format-lib/include/**.h" }
    files { "vendor/ava-format-lib/deps/zlib/*.c", "vendor/ava-format-lib/zlib/*.h" }
    disablewarnings { "4267", "4996" }
group ""