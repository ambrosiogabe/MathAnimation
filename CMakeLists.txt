cmake_minimum_required(VERSION 3.16.0)

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
if (NOT MSVC)
    # MSVC defines _DEBUG and _RELEASE, depending on build type
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/lib)
# Release stuff
if (NOT MSVC)
    # MSVC defines _DEBUG and _RELEASE, depending on build type
    set (CMAKE_C_FLAGS_RELEASE          "${CMAKE_C_FLAGS_RELEASE} -s -D_RELEASE")
    set (CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} -s -D_RELEASE")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_RELEASE")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_RELEASE")
    set (CMAKE_C_FLAGS_MINSIZEREL       "${CMAKE_C_FLAGS_MINSIZEREL} -s -D_RELEASE")
    set (CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} -s -D_RELEASE")
endif ()
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

# Select best available linker on *NIX
if (UNIX AND NOT APPLE)
    include(ProcessorCount)
    ProcessorCount(HOST_PROC_COUNT)

    find_program(MOLD_PROGRAM mold)
    if(MOLD_PROGRAM)
        message(STATUS "Set linker to mold: ${MOLD_PROGRAM}")
        add_link_options("-fuse-ld=mold")
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        # By default LLD uses all system threads.
        # This could be tweaked for versions 11+ (--threads=1), but cannot be disabled for older versions
        # add_link_options("-fuse-ld=lld-${CLANG_VERSION_MAJOR};LINKER:--threads=${HOST_PROC_COUNT}") #LLD>=11
        # add_link_options("-fuse-ld=lld;LINKER:--threads")#LLD <= 10 this is the default state

        string(REPLACE "." ";" VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
        list(GET VERSION_LIST 0 CLANG_VERSION_MAJOR) #extract major compiler version

        find_program(LLD_PROGRAM_MATCH_VER lld-${CLANG_VERSION_MAJOR}) #search for lld-13 when clang 13.x.x is used
        find_program(LLD_PROGRAM lld) #else find default lld

        if (LLD_PROGRAM_MATCH_VER) #lld matching compiler version
            message(STATUS "Set linker to LLD (multi-threaded): ${LLD_PROGRAM_MATCH_VER}")
            add_link_options("-fuse-ld=lld-${CLANG_VERSION_MAJOR}")
        elseif(LLD_PROGRAM) #default lld
            message(STATUS "Set linker to LLD (multi-threaded): ${LLD_PROGRAM}")
            add_link_options("-fuse-ld=lld")
        endif(LLD_PROGRAM_MATCH_VER)
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
        find_program(GNU_GOLD_PROGRAM gold)
        if (GNU_GOLD_PROGRAM)
            message(STATUS "Set linker to GNU gold: ${GNU_GOLD_PROGRAM}, using threads: ${HOST_PROC_COUNT}")
            add_link_options("-fuse-ld=gold;LINKER:--threads,--thread-count=${HOST_PROC_COUNT}")
        endif(GNU_GOLD_PROGRAM)
    endif(MOLD_PROGRAM)
endif(UNIX AND NOT APPLE)

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
    ${THEMES_DIR}/atomOneDark/themes/OneDark.json
    ${THEMES_DIR}/gruvbox/themes/gruvbox-dark-soft.json
    ${THEMES_DIR}/monokaiNight/themes/default.json
    ${THEMES_DIR}/oneMonokai/themes/OneMonokai-color-theme.json
    ${THEMES_DIR}/palenight/themes/palenight.json
    ${THEMES_DIR}/panda/dist/Panda.json
    $<TARGET_FILE_DIR:MathAnimations>/assets/themes
)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if (ipo_supported)
    message(STATUS "IPO / LTO enabled for Release-builds")
    set_property(TARGET MathAnimations PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET MathAnimations PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET MathAnimations PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    if (DEBUG_IPO)
        message(STATUS "IPO / LTO enabled for Debug-builds")
        set_property(TARGET MathAnimations PROPERTY INTERPROCEDURAL_OPTIMIZATION_DEBUG TRUE)
    endif()
else()
    message(STATUS "IPO / LTO not supported: <${ipo_error}>")
endif()

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
if(WIN32)
    file(GLOB_RECURSE nativeFileDialog_SRC
        "Animations/vendor/nativeFileDialog/src/*.h"
        "Animations/vendor/nativeFileDialog/src/include/*.h"
        "Animations/vendor/nativeFileDialog/src/nfd_common.c"
        "Animations/vendor/nativeFileDialog/src/nfd_win.cpp"
    )
else()
    file(GLOB_RECURSE nativeFileDialog_SRC
        "Animations/vendor/nativeFileDialog/src/*.h"
        "Animations/vendor/nativeFileDialog/src/include/*.h"
        "Animations/vendor/nativeFileDialog/src/nfd_common.c"
        "Animations/vendor/nativeFileDialog/src/nfd_gtk.c"
    )
endif()

add_library(NativeFileDialog ${nativeFileDialog_SRC})

if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_link_directories(NativeFileDialog PUBLIC ${GTK3_LIBRARY_DIRS})
    target_include_directories(
        NativeFileDialog PUBLIC
        "./Animations/vendor/nativeFileDialog/src/include"
        ${GTK3_INCLUDE_DIRS}
    )
else()
    target_include_directories(
        NativeFileDialog PUBLIC
        "./Animations/vendor/nativeFileDialog/src/include"
    )
endif()

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

        PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Link Options
# Find FFMpeg stuff
if (MSVC)
    find_library(LIB_AVCODEC libavcodec HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVDEVICE libavdevice HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVFILTER libavfilter HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVFORMAT libavformat HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVUTIL libavutil HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_SWRESAMPLE libswresample HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_SWSCALE libswscale HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
else()
    find_library(LIB_AVCODEC avcodec HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVDEVICE avdevice HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVFILTER avfilter HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVFORMAT avformat HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_AVUTIL avutil HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_SWRESAMPLE swresample HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
    find_library(LIB_SWSCALE swscale HINTS ${CMAKE_SOURCE_DIR}/Animations/vendor/ffmpeg/build/lib)
endif()

if(WIN32)
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
        # FFmpeg stuff
        ${LIB_AVCODEC}
        ${LIB_AVDEVICE}
        ${LIB_AVFILTER}
        ${LIB_AVFORMAT}
        ${LIB_AVUTIL}
        ${LIB_SWRESAMPLE}
        ${LIB_SWSCALE}
        # Windows static libs required for ffmepg
        Ws2_32
        Secur32
        Bcrypt
        Mfuuid
        Strmiids
    )
else()
    target_link_libraries(
        MathAnimations

        ssl
        crypto

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
        # FFmpeg stuff
        va
        vdpau
        X11
        va-x11
        va-drm
        bz2
        z
        ${GTK3_LIBRARIES}
        ${LIB_AVCODEC}
        ${LIB_AVDEVICE}
        ${LIB_AVFILTER}
        ${LIB_AVFORMAT}
        ${LIB_AVUTIL}
        ${LIB_SWRESAMPLE}
        ${LIB_SWSCALE}
        lzma
    )
endif()

if (MSVC)
    set_property(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT MathAnimations
    )
endif()
