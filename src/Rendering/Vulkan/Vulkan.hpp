#pragma once
#ifdef _VULKAN_RENDERING
#include <Vulkan/vulkan.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>
#include <cstdint> // Necessary for uint32_t
#include <cstdlib>
#include <cstring>

#include <algorithm> // Necessary for std::clamp
#include <array>
#include <fstream>
#include <iostream>
#include <limits> // Necessary for std::numeric_limits
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#endif