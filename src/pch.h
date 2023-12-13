#ifndef JCMR_SRC_PCH_H_HEADER_GUARD
#define JCMR_SRC_PCH_H_HEADER_GUARD

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/ext.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <fmt/color.h>
#include <fmt/format.h>

#include "app/log.h"

#define AVA_FL_ERROR_LOG LOG_ERROR
#include <AvaFormatLib/AvaFormatLib.h>

#endif // JCMR_SRC_PCH_H_HEADER_GUARD
