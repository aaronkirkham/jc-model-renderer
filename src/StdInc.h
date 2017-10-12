#pragma once

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

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <ksignals.h>
#include <json.hpp>

namespace fs = std::experimental::filesystem;
using json = nlohmann::json;