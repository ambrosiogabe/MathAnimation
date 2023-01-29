# Configure vendored libraries
set(DEAR_IMGUI_USER_CONFIG ${CMAKE_CURRENT_LIST_DIR}/include/core/InternalImGuiConfig.h)
set(DEAR_IMGUI_EXTRA_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/vendor/cppUtils/single_include
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add all vendored libraries
add_subdirectory(vendor)

##############
# Animations #
##############
set(MATH_ANIMATIONS_ASSETS_DIR ${CMAKE_CURRENT_LIST_DIR}/../assets)
set(MATH_ANIMATIONS_GRAMMARS_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/grammars)
set(MATH_ANIMATIONS_THEMES_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/themes)

set(MATH_ANIMATIONS_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(AV1_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/av1/Source/API)

file(GLOB_RECURSE MATH_ANIMATIONS_SOURCE
    "src/*.cpp"
    "include/*.h"
    "include/*.hpp"
)

if(MATH_ANIMATION_OS_LINUX)
    set(MATH_ANIMATIONS_MAX_PATH 4096)
elseif(MATH_ANIMATION_OS_WINDOWS)
    # Alias to the macro _MAX_PATH on windows
    set(MATH_ANIMATIONS_MAX_PATH _MAX_PATH)
else()
    message(FATAL_ERROR "Targeting unsupported operating system ${CMAKE_SYSTEM_NAME}")
endif()

add_executable(MathAnimations ${MATH_ANIMATIONS_SOURCE})
target_include_directories(MathAnimations PUBLIC ${MATH_ANIMATIONS_INCLUDE_DIR} ${AV1_INCLUDE_DIR})
target_compile_definitions(MathAnimations PUBLIC
        MATH_ANIMATIONS_MAX_PATH=${MATH_ANIMATIONS_MAX_PATH}
)
target_compile_definitions(MathAnimations PRIVATE
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:RelWithDebInfo>:_RELEASE=1>
        $<$<CONFIG:MinSizeRel>:_RELEASE=1>
        $<$<CONFIG:Release>:_RELEASE=1>
        $<$<CONFIG:RelWithProfiler>:_PROFILER=1>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MATH_ANIMATIONS_SOURCE})

target_link_libraries(MathAnimations PUBLIC
    # OpenGL and windowing
    glfw
    glad
    NativeFileDialog

    # Fonts and sound
    freetype
    OpenAL

    # Auxiliary
    CppUtils
    glm
    stb
    nlohmann_json

    # Parsing
    onig
    plutovg
    tinyxml2

    # Dear ImGUI
    DearImGUIGlfwOgl3Backend
    DearImGUI

    # Luau
    Luau.Ast
    Luau.CodeGen
    Luau.Compiler
    Luau.VM
    Luau.Analysis

    # AV1
    SvtAv1Enc

    # Optick
    OptickCore
)

if(MATH_ANIMATION_OS_LINUX)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(MathAnimations PUBLIC OpenSSL::Crypto)
endif()

# Asset processing
add_custom_command(TARGET MathAnimations
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MATH_ANIMATIONS_ASSETS_DIR}
        $<TARGET_FILE_DIR:MathAnimations>/assets

    COMMENT "Copying assets"
    POST_BUILD
)

add_custom_command(TARGET MathAnimations
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:MathAnimations>/assets/grammars
    COMMAND ${CMAKE_COMMAND} -E copy
        ${MATH_ANIMATIONS_GRAMMARS_DIR}/minimalCpp/syntaxes/cpp.tmLanguage.json
        ${MATH_ANIMATIONS_GRAMMARS_DIR}/glsl/syntaxes/glsl.tmLanguage.json
        ${MATH_ANIMATIONS_GRAMMARS_DIR}/javascript/syntaxes/javascript.json
        $<TARGET_FILE_DIR:MathAnimations>/assets/grammars

    COMMENT "Copying grammars"
    POST_BUILD
)

add_custom_command(TARGET MathAnimations
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:MathAnimations>/assets/themes
    COMMAND ${CMAKE_COMMAND} -E copy
        ${MATH_ANIMATIONS_THEMES_DIR}/atomOneDark/themes/OneDark.json
        ${MATH_ANIMATIONS_THEMES_DIR}/gruvbox/themes/gruvbox-dark-soft.json
        ${MATH_ANIMATIONS_THEMES_DIR}/monokaiNight/themes/default.json
        ${MATH_ANIMATIONS_THEMES_DIR}/oneMonokai/themes/OneMonokai-color-theme.json
        ${MATH_ANIMATIONS_THEMES_DIR}/palenight/themes/palenight.json
        ${MATH_ANIMATIONS_THEMES_DIR}/panda/dist/Panda.json
        $<TARGET_FILE_DIR:MathAnimations>/assets/themes

    COMMENT "Copying themes"
    POST_BUILD
)

set_property(TARGET MathAnimations PROPERTY
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MathAnimations>
)
