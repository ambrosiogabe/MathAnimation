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
    cppdialect "C++20"
    staticruntime "on"
    warnings "High"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    buildoptions "/we4062 /wd4201 /DONIG_EXTERN=extern"

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
        "Animations/vendor/plutovg/include",
        "Animations/vendor/dearimgui",
        "Animations/vendor/openal/include",
        "Animations/vendor/nativeFileDialog/src/include",
        "Animations/vendor/nlohmann/single_include",
        "Animations/vendor/onigurama/src",
        -- Luau include dirs
        "Animations/vendor/luau/Compiler/include",
        "Animations/vendor/luau/Common/include",
        "Animations/vendor/luau/VM/include",
        "Animations/vendor/luau/Analysis/include",
        "Animations/vendor/luau/Ast/include"
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
            -- Luau static libs
            "Luau.Ast",
            "Luau.CodeGen",
            "Luau.Compiler",
            "Luau.VM",
            "Luau.Analysis",
            -- Other premake projects
            "DearImGui",
            "TinyXml2",
            "plutovg",
            "Oniguruma",
            -- Windows static libs required for ffmepg
            "Ws2_32.lib",
            "Secur32.lib",
            "Bcrypt.lib",
            "Mfuuid.lib",
            "Strmiids.lib"
        }

    filter { "configurations:Debug" }
        buildoptions "/MDd"
        runtime "Debug"
        symbols "on"

        libdirs {
            "./Animations/vendor/luau/build/Debug"
        }

        postbuildcommands {
            "{COPY} Animations/vendor/openal/build/Debug/OpenAL32.dll %{cfg.targetdir}"
        }

    filter { "configurations:Release" }
        buildoptions "/MD"
        runtime "Release"
        optimize "on"

        defines {
            "_RELEASE"
        }

        libdirs {
            "./Animations/vendor/luau/build/Release"
        }

        postbuildcommands {
            "{COPY} Animations/vendor/openal/build/Release/OpenAL32.dll %{cfg.targetdir}"
        }

    filter { "configurations:Dist" }
        buildoptions "/MD"
        runtime "Release"
        optimize "on"

        defines {
            "_DIST"
        }

        libdirs {
            "./Animations/vendor/luau/build/Release"
        }

        postbuildcommands {
            "{COPY} Animations/vendor/openal/build/Release/OpenAL32.dll %{cfg.targetdir}"
        }

project "plutovg"
    language "C"
    kind "StaticLib"
    staticruntime "on"
    
    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs { 
        "./Animations/vendor/plutovg/include" 
    }

    files { 
        "./Animations/vendor/plutovg/source/*.c",
        "./Animations/vendor/plutovg/source/*.h",
        "./Animations/vendor/plutovg/include/*.h" 
    }

    defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug"
        buildoptions "/MDd"
        defines { "DEBUG" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        buildoptions "/MD"
        defines { "NDEBUG" }
        symbols "Off"
        warnings "Extra"        

project "TinyXml2"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs { 
        "./Animations/vendor/tinyxml2/" 
    }

    files { 
        "./Animations/vendor/tinyxml2/tinyxml2.cpp",
        "./Animations/vendor/tinyxml2/tinyxml2.h" 
    }

    defines { "_CRT_SECURE_NO_WARNINGS" } 

    filter "configurations:Debug"
        buildoptions "/MDd"
        defines { "DEBUG", "NVG_NO_STB" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        buildoptions "/MD"
        defines { "NDEBUG", "NVG_NO_STB" }
        symbols "Off"
        warnings "Extra"    

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
        "./Animations/include"
    }

    defines { "-DIMGUI_USER_CONFIG \"core/InternalImGuiConfig.h\"" }

    filter { "configurations:Debug" }
        buildoptions "/MDd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MD"
        runtime "Release"
        optimize "on"


project "Oniguruma"
    kind "StaticLib"
    language "C"
    cdialect "C11"
    staticruntime "on"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    buildoptions {
        "-DONIG_EXTERN=extern"
    }

    files {
        "./Animations/vendor/onigurama/src/oniguruma.h",
        "./Animations/vendor/onigurama/src/onig-config.in",
        "./Animations/vendor/onigurama/src/regenc.h",
        "./Animations/vendor/onigurama/src/regint.h",
        "./Animations/vendor/onigurama/src/regparse.h",
        "./Animations/vendor/onigurama/src/regcomp.c",
        "./Animations/vendor/onigurama/src/regenc.c",
        "./Animations/vendor/onigurama/src/regerror.c",
        "./Animations/vendor/onigurama/src/regext.c",
        "./Animations/vendor/onigurama/src/regexec.c",
        "./Animations/vendor/onigurama/src/regparse.c",
        "./Animations/vendor/onigurama/src/regsyntax.c",
        "./Animations/vendor/onigurama/src/regtrav.c",
        "./Animations/vendor/onigurama/src/regversion.c",
        "./Animations/vendor/onigurama/src/st.h",
        "./Animations/vendor/onigurama/src/st.c",
        "./Animations/vendor/onigurama/src/oniggnu.h",
        "./Animations/vendor/onigurama/src/reggnu.c",
        "./Animations/vendor/onigurama/src/onigposix.h",
        "./Animations/vendor/onigurama/src/regposerr.c",
        "./Animations/vendor/onigurama/src/regposix.c",
        "./Animations/vendor/onigurama/src/mktable.c",
        "./Animations/vendor/onigurama/src/ascii.c",
        "./Animations/vendor/onigurama/src/euc_jp.c",
        "./Animations/vendor/onigurama/src/euc_tw.c",
        "./Animations/vendor/onigurama/src/euc_kr.c",
        "./Animations/vendor/onigurama/src/sjis.c",
        "./Animations/vendor/onigurama/src/big5.c",
        "./Animations/vendor/onigurama/src/gb18030.c",
        "./Animations/vendor/onigurama/src/koi8.c",
        "./Animations/vendor/onigurama/src/koi8_r.c",
        "./Animations/vendor/onigurama/src/cp1251.c",
        "./Animations/vendor/onigurama/src/iso8859_1.c",
        "./Animations/vendor/onigurama/src/iso8859_2.c",
        "./Animations/vendor/onigurama/src/iso8859_3.c",
        "./Animations/vendor/onigurama/src/iso8859_4.c",
        "./Animations/vendor/onigurama/src/iso8859_5.c",
        "./Animations/vendor/onigurama/src/iso8859_6.c",
        "./Animations/vendor/onigurama/src/iso8859_7.c",
        "./Animations/vendor/onigurama/src/iso8859_8.c",
        "./Animations/vendor/onigurama/src/iso8859_9.c",
        "./Animations/vendor/onigurama/src/iso8859_10.c",
        "./Animations/vendor/onigurama/src/iso8859_11.c",
        "./Animations/vendor/onigurama/src/iso8859_13.c",
        "./Animations/vendor/onigurama/src/iso8859_14.c",
        "./Animations/vendor/onigurama/src/iso8859_15.c",
        "./Animations/vendor/onigurama/src/iso8859_16.c",
        "./Animations/vendor/onigurama/src/utf8.c",
        "./Animations/vendor/onigurama/src/utf16_be.c",
        "./Animations/vendor/onigurama/src/utf16_le.c",
        "./Animations/vendor/onigurama/src/utf32_be.c",
        "./Animations/vendor/onigurama/src/utf32_le.c",
        "./Animations/vendor/onigurama/src/unicode.c",
        "./Animations/vendor/onigurama/src/unicode_unfold_key.c",
        "./Animations/vendor/onigurama/src/unicode_fold1_key.c",
        "./Animations/vendor/onigurama/src/unicode_fold2_key.c",
        "./Animations/vendor/onigurama/src/unicode_fold3_key.c"
    }

    includedirs {
        "./Animations/vendor/onigurama/src",
        "./Animations/vendor/onigurama/build"
    }

    filter { "configurations:Debug" }
        buildoptions "/MDd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MD"
        runtime "Release"
        optimize "on"
