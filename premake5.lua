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
grammarsDir = "./Animations/vendor/grammars/"
themesDir = "./Animations/vendor/themes/"
assetsDir = "./assets/"

project "Animations"
    kind "ConsoleApp"
    language "C++"
    -- C++20 complains about something with incomplete types in Animations/Texture ?
    cppdialect "C++17"
    staticruntime "on"
    warnings "High"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    -- not sure if 4201 has a gcc equivalent?
    filter "system:windows"
        buildoptions "/we4062 /wd4201 /DONIG_EXTERN=extern"
    filter "system:linux"
        buildoptions "-Werror=switch -DONIG_EXTERN=extern -pedantic"

    filter {}

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
        "Animations/vendor/luau/Ast/include",

        "/usr/lib/glib-2.0/include",
        "/usr/include/glib-2.0",
        "/usr/include/pango-1.0",
        "/usr/include/gtk-3.0",
        "/usr/include/gdk-pixbuf-2.0",
        "/usr/include/atk-1.0",
        "/usr/include/harfbuzz",
        "/usr/include/cairo",
    }

    postbuildcommands {
        -- Copy grammars to assets dir
        "{COPYFILE} "..grammarsDir.."minimalCpp/syntaxes/cpp.tmLanguage.json "..assetsDir.."grammars/cpp.tmLanguage.json",
        "{COPYFILE} "..grammarsDir.."glsl/syntaxes/glsl.tmLanguage.json "..assetsDir.."grammars/glsl.tmLanguage.json",
        "{COPYFILE} "..grammarsDir.."javascript/syntaxes/javascript.json "..assetsDir.."grammars/javascript.tmLanguage.json",

        ---- Copy themes to assets dir
        "{COPYFILE} "..themesDir.."atomOneDark/themes/OneDark.json "..assetsDir.."themes/oneDark.theme.json",
        "{COPYFILE} "..themesDir.."gruvbox/themes/gruvbox-dark-soft.json "..assetsDir.."themes/gruvbox.theme.json",
        "{COPYFILE} "..themesDir.."monokaiNight/themes/default.json "..assetsDir.."themes/monokaiNight.theme.json",
        "{COPYFILE} "..themesDir.."oneMonokai/themes/OneMonokai-color-theme.json "..assetsDir.."themes/oneMonokai.theme.json",
        "{COPYFILE} "..themesDir.."palenight/themes/palenight.json "..assetsDir.."themes/palenight.theme.json",
        "{COPYFILE} "..themesDir.."panda/dist/Panda.json "..assetsDir.."themes/panda.theme.json"
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
            "OpenAL32",
        }

    filter "system:linux"
        files {
            -- "Animations/vendor/GLFW/src/wl_init.c",
            -- "Animations/vendor/GLFW/src/wl_monitor.c",
            -- "Animations/vendor/GLFW/src/wl_window.c",
            "Animations/vendor/GLFW/src/x11_init.c",
            "Animations/vendor/GLFW/src/x11_monitor.c",
            "Animations/vendor/GLFW/src/x11_window.c",
            "Animations/vendor/GLFW/src/posix_thread.c",
            "Animations/vendor/GLFW/src/egl_context.c",
            "Animations/vendor/GLFW/src/osmesa_context.c",
            -- NFD
            "./Animations/vendor/nativeFileDialog/src/nfd_gtk.c"
        }

        defines  {
            -- "_GLFW_WAYLAND",
            "_GLFW_X11",
            "_MAX_PATH=261",
            "_CRT_SECURE_NO_WARNINGS"
        }

        includedirs {
            "./Animations/vendor/GLFW/build/src"
        }

        libdirs {
            "./Animations/vendor/ffmpeg/build/lib",
            "./Animations/vendor/GLFW/build/src",
            "./Animations/vendor/onigurama/build/lib"
        }

        links {
            -- ffmpeg static libs
            "avformat",
            "avcodec",
            "avdevice",
            "avfilter",
            "avutil",
            "swresample",
            "swscale",

            "bz2",
            "lzma",

            "openal",
            
            "X11",
            "wayland-client",
            "vdpau",
            "va",
            "va-drm",
            "gtk-3",
            "gdk-3",
            "z",
            "pangocairo-1.0",
            "pango-1.0",
            "harfbuzz",
            "atk-1.0",
            "cairo-gobject",
            "cairo",
            "gdk_pixbuf-2.0",
            "gio-2.0",
            "gobject-2.0",
            "glib-2.0",

            "freetype",

            "ssl",
            "crypto",

            "glfw3",

            "onig",

            -- Luau static libs
            "Luau.Analysis",
            "Luau.Ast",
            "Luau.CodeGen",
            "Luau.Compiler",
            "Luau.VM",
            -- Other premake projects
            "DearImGui",
            "TinyXml2",
            "plutovg",
            "Oniguruma",
        }

    filter { "configurations:Debug" }
        runtime "Debug"
        symbols "on"

        libdirs {
            "./Animations/vendor/luau/build/Debug",
            "./Animations/vendor/openal/build/Debug",
            "./Animations/vendor/freetype/build/Debug",
        }

        postbuildcommands {
            -- "{COPY} Animations/vendor/openal/build/Debug/OpenAL32.dll %{cfg.targetdir}"
        }

    filter { "configurations:Release" }
        runtime "Release"
        optimize "on"

        defines {
            "_RELEASE"
        }

        libdirs {
            "./Animations/vendor/luau/build/Release",
            "./Animations/vendor/openal/build/Release",
            "./Animations/vendor/freetype/build/Release"
        }

        postbuildcommands {
            "{COPY} Animations/vendor/openal/build/Release/OpenAL32.dll %{cfg.targetdir}"
        }

    filter { "configurations:Dist" }
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
        filter "system:windows"
            buildoptions "/MDd"
        defines { "DEBUG" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        filter "system:windows"
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
        "Animations/vendor/tinyxml2/",
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

    files { 
        "./Animations/vendor/tinyxml2/tinyxml2.cpp",
        "./Animations/vendor/tinyxml2/tinyxml2.h" 
    }

    defines { "_CRT_SECURE_NO_WARNINGS" } 

    filter "configurations:Debug"
        filter "system:windows"
            buildoptions "/MDd"
        defines { "DEBUG", "NVG_NO_STB" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        filter "system:windows"
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
    }

    defines { "IMGUI_USER_CONFIG \"core/InternalImGuiConfig.h\"" }

    filter { "configurations:Debug" }
        filter "system:windows"
            buildoptions "/MDd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        filter "system:windows"
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
        "Animations/vendor/onigurama/src",
        "Animations/vendor/onigurama/src",
        "Animations/vendor/onigurama/build",
        "Animations/vendor/tinyxml2/",
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
        -- Luau include dirs
        "Animations/vendor/luau/Compiler/include",
        "Animations/vendor/luau/Common/include",
        "Animations/vendor/luau/VM/include",
        "Animations/vendor/luau/Analysis/include",
        "Animations/vendor/luau/Ast/include"
    }

    filter { "configurations:Debug" }
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        runtime "Release"
        optimize "on"
