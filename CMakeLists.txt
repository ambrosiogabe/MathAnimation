cmake_minimum_required(VERSION 3.15)

project(
    MathAnimationsPrj
    VERSION 1.0
    LANGUAGES C CXX
)

if (MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(GRAMMARS_DIR ${CMAKE_SOURCE_DIR}/Animations/vendor/grammars)
set(THEMES_DIR ${CMAKE_SOURCE_DIR}/Animations/vendor/themes)

# Debug stuff
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/lib)
# Release stuff
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release/lib)

# Prebuild things like copying some assets etc
file(MAKE_DIRECTORY assets/grammars)
file(MAKE_DIRECTORY assets/themes)

# Math Animations
file(GLOB_RECURSE MathAnimations_SRC
    "Animations/src/*.cpp"
    "Animations/include/*.h"
    "Animations/include/*.hpp"
)

file(GLOB_RECURSE glm_SRC 
    "Animations/vendor/glm/glm/*.hpp"
    "Animations/vendor/glm/glm/*.inl"
)

file(GLOB_RECURSE stb_SRC
    "Animations/vendor/stb/stb/*.h"
)

add_executable(
    MathAnimations 
    ${MathAnimations_SRC} 
    ${glm_SRC}
    ${stb_SRC}
)

target_compile_definitions(MathAnimations 
    PUBLIC 
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:_RELEASE>
    $<$<CONFIG:MinSizeRel>:_RELEASE>
    $<$<CONFIG:Release>:_RELEASE>
)

add_custom_command(TARGET MathAnimations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:MathAnimations>/assets
)

if (EXISTS "${CMAKE_SOURCE_DIR}/imgui.ini")
    add_custom_command(TARGET MathAnimations POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/imgui.ini $<TARGET_FILE_DIR:MathAnimations>
    )
endif()

add_custom_command(TARGET MathAnimations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    # Copy Grammars
    ${GRAMMARS_DIR}/minimalCpp/syntaxes/cpp.tmLanguage.json
    ${GRAMMARS_DIR}/glsl/syntaxes/glsl.tmLanguage.json
    ${GRAMMARS_DIR}/javascript/syntaxes/javascript.json
    $<TARGET_FILE_DIR:MathAnimations>/assets/grammars
)

add_custom_command(TARGET MathAnimations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    # Copy Themes
    ${THEMES_DIR}/atomOneDark/themes/oneDark.json
    ${THEMES_DIR}/gruvbox/themes/gruvbox-dark-soft.json
    ${THEMES_DIR}/monokaiNight/themes/default.json
    ${THEMES_DIR}/oneMonokai/themes/OneMonokai-color-theme.json
    ${THEMES_DIR}/palenight/themes/palenight.json
    ${THEMES_DIR}/panda/dist/Panda.json
    $<TARGET_FILE_DIR:MathAnimations>/assets/themes
)

if (MSVC)
    set_property(TARGET MathAnimations PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MathAnimations>
    )

    # CMake hates Visual Studio, so automatically generate the filters
    # so my folder structure is preserved in the IDE
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${glm_SRC})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${stb_SRC})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MathAnimations_SRC})
endif()

target_compile_definitions(
    MathAnimations PUBLIC
)

target_include_directories(
    MathAnimations PUBLIC 
    "Animations/include"
    "Animations/vendor/GLFW/include"
    "Animations/vendor/glad/include"
    "Animations/vendor/cppUtils/single_include/"
    "Animations/vendor/glm/"
    "Animations/vendor/stb/"
    "Animations/vendor/vlc/include"
    "Animations/vendor/ffmpeg/build/include"
    "Animations/vendor/plutovg/include"
    "Animations/vendor/dearimgui"
    "Animations/vendor/openal/include"
    "Animations/vendor/nativeFileDialog/src/include"
    "Animations/vendor/nlohmann/single_include"
    "Animations/vendor/onigurama/src"
    "Animations/vendor/luau/Compiler/include"
    "Animations/vendor/luau/Common/include"
    "Animations/vendor/luau/VM/include"
    "Animations/vendor/luau/Analysis/include"
    "Animations/vendor/luau/Ast/include"
    "Animations/vendor/av1/Source/API"
)

# Dear ImGui backend
set(DearImGuiBackend_SRC
    "Animations/vendor/dearimgui/backends/imgui_impl_opengl3.cpp"
    "Animations/vendor/dearimgui/backends/imgui_impl_opengl3.h"
    "Animations/vendor/dearimgui/backends/imgui_impl_glfw.cpp"
    "Animations/vendor/dearimgui/backends/imgui_impl_glfw.h"
)

add_library(DearImGuiBackend ${DearImGuiBackend_SRC})

target_include_directories(
    DearImGuiBackend PUBLIC 
    "Animations/vendor/dearimgui"
    "Animations/vendor/GLFW/include"
)

# Dear ImGui
file(GLOB DearImGui_SRC
    "Animations/vendor/dearimgui/*.hpp"
    "Animations/vendor/dearimgui/*.cpp"
)

add_library(DearImGui ${DearImGui_SRC})

target_compile_definitions(DearImGui PUBLIC
    IMGUI_USER_CONFIG="core/InternalImGuiConfig.h"
)

target_include_directories(
    DearImGui PUBLIC 
    "Animations/vendor/dearimgui"
    # This is so that my ImVec2 -> Vec2 transformations work
    "Animations/include"
    "Animations/vendor/cppUtils/single_include/"
)

# Glad
set(Glad_SRC
    "Animations/vendor/glad/include/glad/glad.h"
    "Animations/vendor/glad/include/KHR/khrplatform.h"
    "Animations/vendor/glad/src/glad.c"
)

add_library(Glad ${Glad_SRC})

target_include_directories(
    Glad PUBLIC 
    "Animations/vendor/glad/include"
)

# Native File Dialogue
file(GLOB_RECURSE nativeFileDialog_SRC
    "Animations/vendor/nativeFileDialog/src/*.h"
    "Animations/vendor/nativeFileDialog/src/include/*.h"
    "Animations/vendor/nativeFileDialog/src/nfd_common.c"
    "./Animations/vendor/nativeFileDialog/src/nfd_win.cpp"
)

add_library(NativeFileDialog ${nativeFileDialog_SRC})

target_include_directories(
    NativeFileDialog PUBLIC 
    "./Animations/vendor/nativeFileDialog/src/include"
)

# Plutovg
add_library(plutovg STATIC)

add_subdirectory(Animations/vendor/plutovg/include)
add_subdirectory(Animations/vendor/plutovg/source)

# Tinyxml2

set(tinyxml2_SRC
    "Animations/vendor/tinyxml2/tinyxml2.cpp"
    "Animations/vendor/tinyxml2/tinyxml2.h"
)

add_library(tinyxml2 ${tinyxml2_SRC})

target_include_directories(
    tinyxml2 PUBLIC 
    "Animations/vendor/tinyxml2"
)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if (MSVC)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(Animations/vendor/GLFW)

# Luau
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
if (MSVC)
    set(LUAU_STATIC_CRT ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(Animations/vendor/luau)

# OpenAL

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

add_subdirectory(Animations/vendor/openal)

# Freetype

set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

add_subdirectory(Animations/vendor/freetype)

# Oniguruma
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(INSTALL_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_TEST OFF CACHE BOOL "" FORCE)
if(MSVC)
  set(MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(Animations/vendor/onigurama)

# AV1
# Is this important?
#-DCMAKE_INSTALL_PREFIX=%SYSTEMDRIVE%\svt-encoders 
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_AVX512 ON CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)
set(BUILD_DEC OFF CACHE BOOL "" FORCE)
set(CMAKE_ASM_NASM_COMPILER "yasm" CACHE BOOL "" FORCE)
if (MSVC)
    set(MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(Animations/vendor/av1)

# Global properties
set_target_properties(
    MathAnimations
    NativeFileDialog
    Glad
    DearImGui
    DearImGuiBackend
    tinyxml2
    plutovg

    PROPERTIES
    CMAKE_CXX_STANDARD 17
    CMAKE_CXX_STANDARD_REQUIRED True
    LINKER_LANGUAGE CXX 
)

if (MSVC) 
    set_target_properties(
        MathAnimations
        NativeFileDialog
        Glad
        DearImGui
        DearImGuiBackend
        tinyxml2
        plutovg
        OpenAL
        freetype
        common 
        ex-common
        # AV1 Stuff
        SvtAv1Enc
        COMMON_ASM_AVX2
        COMMON_ASM_AVX512
        COMMON_ASM_SSE2
        COMMON_ASM_SSE4_1
        COMMON_ASM_SSSE3
        COMMON_C_DEFAULT
        COMMON_CODEC
        cpuinfo
        FASTFEAT
        ENCODER_ASM_AVX2
        ENCODER_ASM_AVX512
        ENCODER_ASM_SSE2
        ENCODER_ASM_SSE4_1
        ENCODER_ASM_SSSE3
        ENCODER_C_DEFAULT
        ENCODER_CODEC
        ENCODER_GLOBALS
        clog

        PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Link Options
target_link_libraries(
    MathAnimations 
    glfw
    freetype
    DearImGuiBackend
    Glad
    NativeFileDialog
    OpenAL
    Luau.Ast 
    Luau.CodeGen 
    Luau.Compiler 
    Luau.VM 
    Luau.Analysis 
    onig 
    plutovg 
    tinyxml2 
    DearImGui
    SvtAv1Enc
)

if (MSVC)
    set_property(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
        PROPERTY VS_STARTUP_PROJECT MathAnimations
    )
endif()
