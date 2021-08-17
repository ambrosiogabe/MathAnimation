#ifndef MATH_ANIM_CORE_H
#define MATH_ANIM_CORE_H

// Glm
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// My stuff
#include <memory/memory.h>
#include <logger/logger.h>

// Standard
#include <filesystem>
#include <cstring>
#include <iostream>
#include <fstream>
#include <array>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <random>

// GLFW/glad
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// stb
#include <stb/stb_image.h>
#include <stb/stb_write.h>

// User defined literals
glm::vec4 operator "" _hex(const char* hexColor, size_t length);

// Freetype
#include <ft2build.h>
#include FT_FREETYPE_H

#endif
