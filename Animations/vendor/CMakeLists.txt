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
set(CMAKE_CXX_STANDARD 20)
add_library(CppUtils INTERFACE)
target_include_directories(CppUtils INTERFACE ${CMAKE_CURRENT_LIST_DIR}/cppUtils/single_include)
set(CMAKE_CXX_STANDARD 17)

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
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(freetype)

##########
# OpenAL #
##########
set(ALSOFT_DLOPEN OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(LIBTYPE "STATIC")
if (MSVC)
    set(FORCE_STATIC_VCRT ON CACHE BOOL "" FORCE)
endif()
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
add_subdirectory(openal)

########
# Luau #
########
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if (MSVC)
    # Might not do anything, static CRT linking is enabled
    # for the entire CMake setup
    set(LUAU_STATIC_CRT ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(luau)

#############
# Onigurama #
#############
set(INSTALL_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_TEST OFF CACHE BOOL "" FORCE)
if(MSVC)
    set(MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(onigurama)

############
# TinyXML2 #
############
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(tinyxml2)

###########
# PlutoVG #
###########
add_library(plutovg STATIC)

add_subdirectory(plutovg/include)
add_subdirectory(plutovg/source)

#############
# dearimgui #
#############
include(DearImGUI.cmake)

########
# Glad #
########
add_subdirectory(glad)

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
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb)

#######
# AV1 #
#######
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_AVX512 ON CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)
set(BUILD_DEC OFF CACHE BOOL "" FORCE)
set(CMAKE_ASM_NASM_COMPILER "yasm" CACHE BOOL "" FORCE)
if (MSVC)
    set(MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(av1)


##########
# Optick #
##########
set(OPTICK_INSTALL_TARGETS OFF)
set(OPTICK_ENABLED ON)

add_subdirectory(optick)


##############
# Rapid Fuzz #
##############
add_subdirectory(rapidFuzz)
