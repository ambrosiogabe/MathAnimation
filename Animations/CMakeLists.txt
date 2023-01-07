# Configure vendored libraries
set(DEAR_IMGUI_USER_CONFIG "${CMAKE_CURRENT_LIST_DIR}/include/core/InternalImGuiConfig.h")
set(DEAR_IMGUI_EXTRA_INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/vendor/cppUtils/single_include"
        "${CMAKE_CURRENT_LIST_DIR}/include")

# Add all vendored libraries
add_subdirectory(vendor)

##############
# Animations #
##############
# TODO: Project should be restructured to avoid accessing files outside the scope
set(MATH_ANIMATIONS_ASSETS_DIR "${CMAKE_CURRENT_LIST_DIR}/../assets")
set(MATH_ANIMATIONS_GRAMMARS_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor/grammars")
set(MATH_ANIMATIONS_THEMES_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor/themes")

set(MATH_ANIMATIONS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

set(MATH_ANIMATIONS_SOURCE
        src/multithreading/GlobalThreadPool.cpp

        src/video/Encoder.cpp
        src/video/NvidiaEncoder.cpp

        src/parsers/SyntaxHighlighter.cpp
        src/parsers/Grammar.cpp
        src/parsers/Common.cpp
        src/parsers/SyntaxTheme.cpp

        src/animation/Axis.cpp
        src/animation/SvgFileObject.cpp
        src/animation/TextAnimations.cpp
        src/animation/AnimationManager.cpp
        src/animation/Shapes.cpp
        src/animation/Animation.cpp

        src/scripting/MathAnimGlobals.cpp
        src/scripting/LuauLayer.cpp
        src/scripting/GlobalApi.cpp
        src/scripting/ScriptAnalyzer.cpp

        src/audio/Audio.cpp
        src/audio/WavLoader.cpp

        src/editor/Timeline.cpp
        src/editor/ProjectScreen.cpp
        src/editor/ImGuiExtended.cpp
        src/editor/EditorCameraController.cpp
        src/editor/MenuBar.cpp
        src/editor/EditorGui.cpp
        src/editor/ImGuiTimeline.cpp
        src/editor/DebugPanel.cpp
        src/editor/SceneHierarchyPanel.cpp
        src/editor/ExportPanel.cpp
        src/editor/AnimObjectPanel.cpp
        src/editor/EditorSettings.cpp
        src/editor/ConsoleLog.cpp
        src/editor/SceneManagementPanel.cpp
        src/editor/AssetManagerPanel.cpp
        src/editor/Gizmos.cpp

        src/svg/Paths.cpp
        src/svg/TexturePacker.cpp
        src/svg/SvgCache.cpp
        src/svg/Svg.cpp
        src/svg/SvgParser.cpp

        src/renderer/Shader.cpp
        src/renderer/PerspectiveCamera.cpp
        src/renderer/Renderer.cpp
        src/renderer/Texture.cpp
        src/renderer/Colors.cpp
        src/renderer/Fonts.cpp
        src/renderer/OrthoCamera.cpp
        src/renderer/Framebuffer.cpp

        src/utils/CMath.cpp
        src/utils/TableOfContents.cpp

        src/math/DataStructures.cpp

        src/core/ImGuiLayer.cpp
        src/core/Input.cpp
        src/core/ProjectApp.cpp
        src/core/Application.cpp
        src/core/GladLayer.cpp
        src/core/Window.cpp

        src/latex/LaTexLayer.cpp

        src/SingleIncludeImpls.cpp
        src/core.cpp
        src/main.cpp)

if(MATH_ANIMATION_OS_LINUX)
    set(MATH_ANIMATIONS_PLATFORM_SOURCE
            src/platform/PlatformLinux.cpp
            src/platform/FileSystemWatcherLinux.cpp)

    # TODO: The source code used to use _MAX_PATH, but this can cause issues on Linux
    # see: https://stackoverflow.com/questions/833291/is-there-an-equivalent-to-winapis-max-path-under-linux-unix
    set(MATH_ANIMATIONS_MAX_PATH 4096)
elseif(MATH_ANIMATION_OS_WINDOWS)
    set(MATH_ANIMATIONS_PLATFORM_SOURCE
            src/platform/FileSystemWatcherWin32.cpp
            src/platform/PlatformWin32.cpp)

    # Alias to the macro _MAX_PATH on windows
    set(MATH_ANIMATIONS_MAX_PATH _MAX_PATH)
else()
    message(FATAL_ERROR "Targeting unsupported operating system ${CMAKE_SYSTEM_NAME}")
endif()

add_executable(MathAnimations ${MATH_ANIMATIONS_SOURCE} ${MATH_ANIMATIONS_PLATFORM_SOURCE})
target_include_directories(MathAnimations PUBLIC ${MATH_ANIMATIONS_INCLUDE_DIR})
target_compile_definitions(MathAnimations PUBLIC MATH_ANIMATIONS_MAX_PATH=${MATH_ANIMATIONS_MAX_PATH})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${MATH_ANIMATIONS_SOURCE} ${MATH_ANIMATIONS_PLATFORM_SOURCE})

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

        # FFMPEG
        ffmpeg::avcodec
        ffmpeg::avdevice
        ffmpeg::avfilter
        ffmpeg::avformat
        ffmpeg::avutil
        ffmpeg::swresample
        ffmpeg::swscale
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
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MathAnimations>)
