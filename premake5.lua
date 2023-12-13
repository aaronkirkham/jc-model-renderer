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
  defines { "WIN32", "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING", "IMGUI_DISABLE_OBSOLETE_FUNCTIONS" }

  filter "configurations:Debug"
    defines { "DEBUG", "_DEBUG", "_ITERATOR_DEBUG_LEVEL=0" }
    symbols "On"

  filter "configurations:Release"
    optimize "On"

project "jc-model-renderer"
  kind "consoleapp"
  defines "CPPHTTPLIB_ZLIB_SUPPORT"
  disablewarnings { "4003", "4200", "4244", "4267", "4309", "6031", "6262" }
  dependson { "imgui", "zlib", "tinyxml2", "AvaFormatLib" }
  postbuildcommands { "{COPY} %{cfg.buildtarget.relpath} %{prj.location}../" }
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
    "fmt",
    "AvaFormatLib"
  }
  files {
    "src/assets.rc",
    "src/**.h",
    "src/**.cc"
  }
  includedirs {
    "src",
    "vendor/argparse",
    "vendor/glm",
    "vendor/httplib",
    "vendor/imgui",
    "vendor/rapidjson/include",
    "vendor/spdlog/include",
    "vendor/tinyxml2",
    "vendor/fmt/include",
    "vendor/ava-format-lib/include",
    "vendor/ava-format-lib/deps/zlib"
  }
  filter "configurations:Debug*"
    targetname "jc-model-renderer-d"

group "vendor"
  project "imgui"
    kind "StaticLib"
    defines "IMGUI_DISABLE_OBSOLETE_FUNCTIONS"
    files {
      "vendor/imgui/*.h",
      "vendor/imgui/*.cpp",
      "vendor/imgui/backends/imgui_impl_win32.cpp",
      "vendor/imgui/backends/imgui_impl_win32.h",
      "vendor/imgui/backends/imgui_impl_dx11.cpp",
      "vendor/imgui/backends/imgui_impl_dx11.h",
      "vendor/imgui/misc/cpp/imgui_stdlib.cpp",
      "vendor/imgui/misc/cpp/imgui_stdlib.h"
    }

    includedirs {
      "vendor/imgui",
      "vendor/imgui/backends",
      "vendor/imgui/misc/cpp"
    }

  project "tinyxml2"
    kind "StaticLib"
    files { "vendor/tinyxml2/tinyxml2.cpp", "vendor/tinyxml2/tinyxml2.h" }
    includedirs { "vendor/tinyxml2" }

  project "fmt"
    kind "staticlib"
    language "c++"
    cppdialect "c++17"
    files {
      "vendor/fmt/src/format.cc",
      "vendor/fmt/src/os.cc",
      "vendor/fmt/include/fmt/*.h"
    }
    includedirs {
      "vendor/fmt/include"
    }

  project "AvaFormatLib"
    kind "StaticLib"
    includedirs { "vendor/ava-format-lib/include/AvaFormatLib", "vendor/ava-format-lib/deps" }
    files { "vendor/ava-format-lib/src/**.cpp", "vendor/ava-format-lib/include/**.h" }
    files { "vendor/ava-format-lib/deps/zlib/*.c", "vendor/ava-format-lib/zlib/*.h" }
    disablewarnings { "4267", "4996", "26819" }
group ""