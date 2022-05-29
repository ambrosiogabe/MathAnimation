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
        "./Animations/vendor/dearimgui/backends/imgui_impl_glfw.h"
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
        "./Animations/vendor/imguizmo"
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
        }

        defines  {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

        libdirs {
            "./Animations/vendor/ffmpeg/build/lib",
            "\"./Animations/vendor/freetype/release dll/win64\""
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
            -- Working on this
            "freetype.lib",
            "nanovg",
            "DearImGui",
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
        defines { "DEBUG", "NVG_NO_STB" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
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

project "Bootstrap"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir("_bin/" .. outputdir .. "/%{prj.name}")
    objdir("_bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Bootstrap/src/**.cpp",
        "Bootstrap/include/**.h",
        "Bootstrap/vendor/bit7z/include/**.hpp"
    }

    includedirs {
        "Bootstrap/include",
        "Bootstrap/vendor/curl/include",
        "Animations/vendor/logger/single_include/",
        "Animations/vendor/memory/single_include/",
        "Bootstrap/vendor/bit7z/include/"
    }

    filter "system:windows"
        buildoptions { "-lgdi32" }
        systemversion "latest"

        libdirs {
            "./Bootstrap/vendor/curl/lib",
            "./Bootstrap/vendor/bit7z/lib"
        }

        links {
            "libcurl.dll.lib",
            "oleaut32",
            "user32"
        }

        filter { "configurations:Debug" }
            links {
                "bit7z64_d"
            }
        filter { "configurations:Release" }
            links {
                "bit7z64"
            }

        defines  {
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter { "configurations:Debug" }
        buildoptions "/MTd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MT"
        runtime "Release"
        optimize "on"