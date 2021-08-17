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
        "Animations/vendor/stb/stb_image.h"
    }

    includedirs {
        "Animations/include",
        "Animations/vendor/GLFW/include",
        "Animations/vendor/glad/include",
        "Animations/vendor/logger/single_include/",
        "Animations/vendor/memory/single_include/",
        "Animations/vendor/glm/",
        "Animations/vendor/stb/",
        "Animations/vendor/vlc/include",
        "Animations/vendor/ffmpeg/include",
        "Animations/vendor/freetype/include"
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
            "./Animations/vendor/ffmpeg/lib",
            "\"./Animations/vendor/freetype/release dll/win64\""
        }

        links {
            "avcodec.lib",
            "avdevice.lib",
            "avfilter.lib",
            "avformat.lib",
            "avutil.lib",
            "postproc.lib",
            "swresample.lib",
            "swscale.lib",
            "freetype.lib"
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