workspace "MathAnimations"
    architecture "x64"

    configurations {
        "Debug",
        "Release",
        "Dist"
    }

    startproject "Animations"

-- This is a helper variable, to concatenate the sys-arch
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Animations"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Animations/src/**.cpp",
        "Animations/include/**.h",
        "Animations/vendor/GLFW/include/GLFW/glfw3.h",
        "Animations/vendor/GLFW/include/GLFW/glfw3native.h",
        "Animations/vendor/GLFW/src/glfw_config.h",
        "Animations/vendor/GLFW/src/context.c",
        "Animations/vendor/GLFW/src/init.c",
        "Animations/vendor/GLFW/src/input.c",
        "Animations/vendor/GLFW/src/monitor.c",
        "Animations/vendor/GLFW/src/vulkan.c",
        "Animations/vendor/GLFW/src/window.c",
        "Animations/vendor/glad/include/glad/glad.h",
        "Animations/vendor/glad/include/glad/KHR/khrplatform.h",
		"Animations/vendor/glad/src/glad.c",
        "Animations/vendor/glm/glm/**.hpp",
		"Animations/vendor/glm/glm/**.inl",
        "Animations/vendor/stb/stb_image.h",
        -- Compile ImGui backends into the main code
        "./Animations/vendor/dearimgui/backends/imgui_impl_opengl3.cpp",
        "./Animations/vendor/dearimgui/backends/imgui_impl_opengl3.h",
        "./Animations/vendor/dearimgui/backends/imgui_impl_glfw.cpp",
        "./Animations/vendor/dearimgui/backends/imgui_impl_glfw.h",
        "./Animations/vendor/nativeFileDialog/src/*.h",
        "./Animations/vendor/nativeFileDialog/src/include/*.h",
        "./Animations/vendor/nativeFileDialog/src/nfd_common.c",
        "Animations/vendor/microtex/src/latex.h",
        "Animations/vendor/microtex/src/render.h"
    }

    includedirs {
        "Animations/include",
        "Animations/vendor/GLFW/include",
        "Animations/vendor/glad/include",
        "Animations/vendor/cppUtils/single_include/",
        "Animations/vendor/glm/",
        "Animations/vendor/stb/",
        "Animations/vendor/vlc/include",
        "Animations/vendor/ffmpeg/build/include",
        "Animations/vendor/freetype/include",
        "Animations/vendor/nanovg/src",
        "Animations/vendor/dearimgui",
        -- TODO: Remove this
        "./Animations/vendor/imguizmo",
        "Animations/vendor/openal/include",
        "Animations/vendor/nativeFileDialog/src/include",
    }

    -- Add freetype libdirs for debug and release mode
    filter { "configurations:Debug", "system:windows" }
        libdirs {
            "./Animations/vendor/freetype/build/Debug",
            "./Animations/vendor/openal/build/Debug"
        }

        links {
            -- Freetype
            "freetyped",
            "OpenAL32"
        }

    filter { "configurations:Release", "system:windows" }
        libdirs {
            "./Animations/vendor/freetype/build/Release",
            "./Animations/vendor/openal/build/Release"
        }
        
        links {
            -- Freetype
            "freetype",
            "OpenAL32"
        }

    filter "system:windows"
        buildoptions { "-lgdi32" }
        systemversion "latest"

        files {
            "Animations/vendor/GLFW/src/win32_init.c",
            "Animations/vendor/GLFW/src/win32_joystick.c",
            "Animations/vendor/GLFW/src/win32_monitor.c",
            "Animations/vendor/GLFW/src/win32_time.c",
            "Animations/vendor/GLFW/src/win32_thread.c",
            "Animations/vendor/GLFW/src/win32_window.c",
            "Animations/vendor/GLFW/src/wgl_context.c",
            "Animations/vendor/GLFW/src/egl_context.c",
            "Animations/vendor/GLFW/src/osmesa_context.c",
            -- NFD
            "./Animations/vendor/nativeFileDialog/src/nfd_win.cpp"
        }

        defines  {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

        libdirs {
            "./Animations/vendor/ffmpeg/build/lib"
        }

        links {
            -- ffmpeg static libs
            "libavcodec",
            "libavdevice",
            "libavfilter",
            "libavformat",
            "libavutil",
            "libswresample",
            "libswscale",
            -- Other premake projects
            "nanovg",
            "DearImGui",
            "MicroTex",
            -- Windows static libs required for ffmepg
            "Ws2_32.lib",
            "Secur32.lib",
            "Bcrypt.lib",
            "Mfuuid.lib",
            "Strmiids.lib"
        }

    filter { "configurations:Debug" }
        buildoptions "/MTd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MT"
        runtime "Release"
        optimize "on"

        defines {
            "_RELEASE"
        }

project "nanovg"
    language "C"
    kind "StaticLib"
    staticruntime "on"
    
    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs { 
        "./Animations/vendor/nanovg/src" 
    }

    files { 
        "./Animations/vendor/nanovg/src/*.c" 
    }

    defines { "_CRT_SECURE_NO_WARNINGS" } --,"FONS_USE_FREETYPE" } Uncomment to compile with FreeType support

    filter "configurations:Debug"
        buildoptions "/MTd"
        defines { "DEBUG", "NVG_NO_STB" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        buildoptions "/MT"
        defines { "NDEBUG", "NVG_NO_STB" }
        symbols "Off"
        warnings "Extra"

project "MicroTex"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    -- Taken from microtex CMake
    files {
        -- atom folder
        "Animations/vendor/microtex/src/atom/atom_basic.cpp",
        "Animations/vendor/microtex/src/atom/atom_char.cpp",
        "Animations/vendor/microtex/src/atom/atom_impl.cpp",
        "Animations/vendor/microtex/src/atom/atom_matrix.cpp",
        "Animations/vendor/microtex/src/atom/atom_row.cpp",
        "Animations/vendor/microtex/src/atom/atom_space.cpp",
        "Animations/vendor/microtex/src/atom/colors_def.cpp",
        "Animations/vendor/microtex/src/atom/unit_conversion.cpp",
        -- box folder
        "Animations/vendor/microtex/src/box/box.cpp",
        "Animations/vendor/microtex/src/box/box_factory.cpp",
        "Animations/vendor/microtex/src/box/box_group.cpp",
        "Animations/vendor/microtex/src/box/box_single.cpp",
        -- core folder
        "Animations/vendor/microtex/src/core/core.cpp",
        "Animations/vendor/microtex/src/core/formula.cpp",
        "Animations/vendor/microtex/src/core/formula_def.cpp",
        "Animations/vendor/microtex/src/core/glue.cpp",
        "Animations/vendor/microtex/src/core/localized_num.cpp",
        "Animations/vendor/microtex/src/core/macro.cpp",
        "Animations/vendor/microtex/src/core/macro_def.cpp",
        "Animations/vendor/microtex/src/core/macro_impl.cpp",
        "Animations/vendor/microtex/src/core/parser.cpp",
        -- fonts folder
        "Animations/vendor/microtex/src/fonts/alphabet.cpp",
        "Animations/vendor/microtex/src/fonts/font_basic.cpp",
        "Animations/vendor/microtex/src/fonts/font_info.cpp",
        "Animations/vendor/microtex/src/fonts/fonts.cpp",
        -- utils folder
        "Animations/vendor/microtex/src/utils/string_utils.cpp",
        "Animations/vendor/microtex/src/utils/utf.cpp",
        "Animations/vendor/microtex/src/utils/utils.cpp",
        -- res folder
        "Animations/vendor/microtex/src/res/builtin/formula_mappings.res.cpp",
        "Animations/vendor/microtex/src/res/builtin/symbol_mapping.res.cpp",
        "Animations/vendor/microtex/src/res/builtin/tex_param.res.cpp",
        "Animations/vendor/microtex/src/res/builtin/tex_symbols.res.cpp",
        "Animations/vendor/microtex/src/res/font/bi10.def.cpp",
        "Animations/vendor/microtex/src/res/font/bx10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmbsy10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmbx10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmbxti10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmex10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmmi10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmmi10_unchanged.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmmib10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmmib10_unchanged.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmr10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmss10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmssbx10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmssi10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmsy10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmti10.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmti10_unchanged.def.cpp",
        "Animations/vendor/microtex/src/res/font/cmtt10.def.cpp",
        "Animations/vendor/microtex/src/res/font/dsrom10.def.cpp",
        "Animations/vendor/microtex/src/res/font/eufb10.def.cpp",
        "Animations/vendor/microtex/src/res/font/eufm10.def.cpp",
        "Animations/vendor/microtex/src/res/font/i10.def.cpp",
        "Animations/vendor/microtex/src/res/font/moustache.def.cpp",
        "Animations/vendor/microtex/src/res/font/msam10.def.cpp",
        "Animations/vendor/microtex/src/res/font/msbm10.def.cpp",
        "Animations/vendor/microtex/src/res/font/r10.def.cpp",
        "Animations/vendor/microtex/src/res/font/r10_unchanged.def.cpp",
        "Animations/vendor/microtex/src/res/font/rsfs10.def.cpp",
        "Animations/vendor/microtex/src/res/font/sb10.def.cpp",
        "Animations/vendor/microtex/src/res/font/sbi10.def.cpp",
        "Animations/vendor/microtex/src/res/font/si10.def.cpp",
        "Animations/vendor/microtex/src/res/font/special.def.cpp",
        "Animations/vendor/microtex/src/res/font/ss10.def.cpp",
        "Animations/vendor/microtex/src/res/font/stmary10.def.cpp",
        "Animations/vendor/microtex/src/res/font/tt10.def.cpp",
        "Animations/vendor/microtex/src/res/parser/font_parser.cpp",
        "Animations/vendor/microtex/src/res/parser/formula_parser.cpp",
        "Animations/vendor/microtex/src/res/reg/builtin_font_reg.cpp",
        "Animations/vendor/microtex/src/res/reg/builtin_syms_reg.cpp",
        "Animations/vendor/microtex/src/res/sym/amsfonts.def.cpp",
        "Animations/vendor/microtex/src/res/sym/amssymb.def.cpp",
        "Animations/vendor/microtex/src/res/sym/base.def.cpp",
        "Animations/vendor/microtex/src/res/sym/stmaryrd.def.cpp",
        "Animations/vendor/microtex/src/res/sym/symspecial.def.cpp",

        "Animations/vendor/microtex/src/latex.cpp",
        "Animations/vendor/microtex/src/render.cpp",

        -- TinyXml2 (added by me)
        "Animations/vendor/tinyxml2/tinyxml2.cpp",
        "Animations/vendor/tinyxml2/tinyxml2.h"
    }

    includedirs {
        "Animations/vendor/microtex/src",
        "Animations/vendor/tinyxml2"
    }

    buildoptions { 
        "/utf-8" 
    }

    defines {
        "BUILD_WIN32",
        "_HAS_STD_BYTE=0"
    }

    filter "configurations:Debug"
        buildoptions "/MTd"
        symbols "On"

    filter "configurations:Release"
        buildoptions "/MT"
        symbols "Off"

project "DearImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "./Animations/vendor/dearimgui/*.hpp",
        "./Animations/vendor/dearimgui/*.cpp",
        -- Package ImGuizmo into here as well
        "./Animations/vendor/imguizmo/*.cpp",
        "./Animations/vendor/imguizmo/*.hpp"
    }

    includedirs {
        "./Animations/vendor/dearimgui",
        "./Animations/vendor/imguizmo"
    }

    filter { "configurations:Debug" }
        buildoptions "/MTd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MT"
        runtime "Release"
        optimize "on"
