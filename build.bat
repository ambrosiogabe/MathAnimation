@echo off

IF "%~1" == "" GOTO PrintHelp
IF "%~1" == "help" GOTO PrintHelp
IF "%~1" == "h" GOTO PrintHelp
IF "%~1" == "create" GOTO Create

REM Build the project files
vendor\premake5.exe %1
GOTO Done

:PrintHelp

echo.
echo Enter 'build.bat action' where action is one of the following:
echo.
echo   codelite          Generate CodeLite project files
echo   gmake             Generate GNU makefiles for Linux
echo   vs2005            Generate Visual Studio 2005 project files
echo   vs2008            Generate Visual Studio 2008 project files
echo   vs2010            Generate Visual Studio 2010 project files
echo   vs2012            Generate Visual Studio 2012 project files
echo   vs2013            Generate Visual Studio 2013 project files
echo   vs2015            Generate Visual Studio 2015 project files
echo   vs2017            Generate Visual Studio 2017 project files
echo   vs2019            Generate Visual Studio 2019 project files
echo   xcode4            Generate Apple Xcode 4 project files
GOTO Done

:Create

if "%~2" == "" GOTO PrintCreateHelp

mkdir %2%
mkdir %2%\vendor
mkdir %2%\src
mkdir %2%\include
mkdir %2%\vendor\GLFW
mkdir %2%\vendor\glad

pushd %2%\vendor\GLFW

git init
git pull https://github.com/glfw/glfw

popd

mkdir tmp
pushd tmp

git init
git pull https://github.com/Dav1dde/glad
python -m glad --generator c --profile core --api gl=3.3 --out-path ../%2%/vendor/glad

popd

rmdir tmp /s /q

REM Create premake file to generate solution
(
echo workspace "%~2"
echo     architecture "x64"
echo.
echo     configurations {
echo         "Debug",
echo         "Release",
echo         "Dist"
echo     }
echo.
echo     startproject "%~2"
echo.
echo -- This is a helper variable, to concatenate the sys-arch
echo outputdir = "%%{cfg.buildcfg}-%%{cfg.system}-%%{cfg.architecture}"
echo.
echo project "%~2"
echo     kind "ConsoleApp"
echo     language "C++"
echo     cppdialect "C++17"
echo     staticruntime "on"
echo.
echo     targetdir("bin/" .. outputdir .. "/%{prj.name}"^)
echo     objdir("bin-int/" .. outputdir .. "/%{prj.name}"^)
echo.
echo     files {
echo         "%~2/src/**.cpp",
echo         "%~2/include/**.h",
echo         "%~2/vendor/GLFW/include/GLFW/glfw3.h",
echo         "%~2/vendor/GLFW/include/GLFW/glfw3native.h",
echo         "%~2/vendor/GLFW/src/glfw_config.h",
echo         "%~2/vendor/GLFW/src/context.c",
echo         "%~2/vendor/GLFW/src/init.c",
echo         "%~2/vendor/GLFW/src/input.c",
echo         "%~2/vendor/GLFW/src/monitor.c",
echo         "%~2/vendor/GLFW/src/vulkan.c",
echo         "%~2/vendor/GLFW/src/window.c",
echo         "%~2/vendor/glad/include/glad/glad.h",
echo         "%~2/vendor/glad/include/glad/KHR/khrplatform.h",
echo 		"%~2/vendor/glad/src/glad.c"
echo     }
echo.
echo     includedirs {
echo         "%~2/include",
echo         "%~2/vendor/GLFW/include",
echo         "%~2/vendor/glad/include"
echo     }
echo.
echo     filter "system:windows"
echo         buildoptions { "-lgdi32" }
echo         systemversion "latest"
echo.
echo         files {
echo             "%~2/vendor/GLFW/src/win32_init.c",
echo             "%~2/vendor/GLFW/src/win32_joystick.c",
echo             "%~2/vendor/GLFW/src/win32_monitor.c",
echo             "%~2/vendor/GLFW/src/win32_time.c",
echo             "%~2/vendor/GLFW/src/win32_thread.c",
echo             "%~2/vendor/GLFW/src/win32_window.c",
echo             "%~2/vendor/GLFW/src/wgl_context.c",
echo             "%~2/vendor/GLFW/src/egl_context.c",
echo             "%~2/vendor/GLFW/src/osmesa_context.c"
echo         }
echo.
echo         defines  {
echo             "_GLFW_WIN32",
echo             "_CRT_SECURE_NO_WARNINGS"
echo         }
echo.
echo     filter { "configurations:Debug" }
echo         buildoptions "/MTd"
echo         runtime "Debug"
echo         symbols "on"
echo.
echo     filter { "configurations:Release" }
echo         buildoptions "/MT"
echo         runtime "Release"
echo         optimize "on"
echo.
) > premake5.lua

REM Create temporary header
SET hFile=%2%\include\main.h
(
echo #include ^<stdio.h^>
echo #include ^<glad/glad.h^>
echo #include ^<GLFW/glfw3.h^>
echo.
echo int main(^)
echo {
echo    printf("Hello OpenGL\n"^);
echo.
echo    glfwInit(^);
echo    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3^);
echo    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3^);
echo    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE^);
echo.
echo    const int windowWidth = 1920;
echo    const int windowHeight = 1080;
echo    const char* windowTitle = "OpenGL Template";
echo    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr^);
echo    if (window == nullptr^)
echo    {
echo 	   printf("Failed to create GLFW window\n"^);
echo 	   glfwTerminate(^);
echo 	   return -1;
echo    }
echo    glfwMakeContextCurrent(window^);
echo.
echo    if (!gladLoadGLLoader((GLADloadproc^)glfwGetProcAddress^)^)
echo    {
echo 	   printf("Failed to initialize GLAD.\n"^);
echo 	   return -1;
echo    }
echo.
echo    glViewport(0, 0, windowWidth, windowHeight^);
echo    while (!glfwWindowShouldClose(window^)^)
echo    {
echo 	   glClearColor(250.0f / 255.0f, 119.0f / 255.0f, 110.0f / 255.0f, 1.0f^);
echo 	   glClear(GL_COLOR_BUFFER_BIT^);
echo.
echo 	   glfwSwapBuffers(window^);
echo 	   glfwPollEvents(^);
echo    }
echo.
echo    glfwTerminate(^);
echo    return 0;
echo }
echo.
) > %hFile%

REM Create small OpenGL application
SET cppFile=%2%\src\main.cpp
(
echo #include "main.h"
echo.
) > %cppFile%

GOTO Done

:PrintCreateHelp

echo.
echo To create a new OpenGL project, make sure to have git installed and available on the command line.
echo Then open a command prompt in this directory and type:
echo.
echo   build.bat create myProject
echo.
echo Where 'myProject' is the name of the project you would like to create.
echo.

:Done