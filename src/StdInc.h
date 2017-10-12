#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d3d11.h>

#include <cstdint>
#include <memory>
#include <assert.h>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <fstream>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ksignals/ksignals.h>
#include <json.hpp>

namespace fs = std::experimental::filesystem;
namespace json = nlohmann::json;