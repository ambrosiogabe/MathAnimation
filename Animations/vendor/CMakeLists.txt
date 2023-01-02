# Vendored libraries

# This file acts as a proxy between all the vendored libraries
# and the CMake build system.

###############
# CMake setup #
###############
include(ExternalProject)

############
# cppUtils #
############
add_library(CppUtils INTERFACE)
target_include_directories(CppUtils INTERFACE "${CMAKE_CURRENT_LIST_DIR}/cppUtils/single_include")

########
# GLFW #
########
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(GLFW)

############
# Freetype #
############
add_subdirectory(freetype)

##########
# OpenAL #
##########
set(ALSOFT_UTILS OFF)
set(ALSOFT_NO_CONFIG_UTIL ON)
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_INSTALL OFF)
set(ALSOFT_INSTALL_CONFIG OFF)
set(ALSOFT_INSTALL_HRTF_DATA OFF)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF)
set(ALSOFT_INSTALL_EXAMPLES OFF)
set(ALSOFT_INSTALL_UTILS OFF)
add_subdirectory(openal)

########
# Luau #
########
set(LUAU_BUILD_CLI OFF)
set(LUAU_BUILD_TESTS OFF)
add_subdirectory(luau)

#############
# Onigurama #
#############
add_subdirectory(onigurama)

############
# TinyXML2 #
############
add_subdirectory(tinyxml2)

###########
# PlutoVG #
###########
add_subdirectory(plutovg)

#############
# dearimgui #
#############
include(DearImGUI.cmake)

##########
# ffmpeg #
##########
include(ffmpeg.cmake)

########
# Glad #
########
add_subdirectory(glad/cmake)
glad_add_library(glad STATIC API gl:core=4.2)

#######
# GLM #
#######
add_subdirectory(glm)

####################
# NativeFileDialog #
####################
include(NativeFileDialog.cmake)

############
# nlohmann #
############
add_subdirectory(nlohmann)

#######
# STB #
#######
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_LIST_DIR}/stb")
