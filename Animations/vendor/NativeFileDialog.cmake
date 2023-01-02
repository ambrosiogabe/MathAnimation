# Native file dialog build support
cmake_dependent_option(NATIVE_FILE_DIALOG_USE_GTK "Use GTK-3 instead of Zenity" ON MATH_ANIMATION_OS_LINUX OFF)

set(NATIVE_FILE_DIALOG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/nativeFileDialog/src/include")
set(NATIVE_FILE_DIALOG_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/nativeFileDialog/src")

set(NATIVE_FILE_DIALOG_SOURCE
        "nativeFileDialog/src/nfd_common.c")

if (MATH_ANIMATION_OS_LINUX)
    if (NATIVE_FILE_DIALOG_USE_GTK)
        message(STATUS "Using GTK backend for NativeFileDialog")
        set(NATIVE_FILE_DIALOG_PLATFORM_SOURCE
                "nativeFileDialog/src/nfd_gtk.c")
    else()
        message(STATUS "Using zenity backend for NativeFileDialog")
        set(NATIVE_FILE_DIALOG_PLATFORM_SOURCE
                "nativeFileDialog/src/nfd_zenity.c")
    endif()
elseif(MATH_ANIMATION_OS_WINDOWS)
    set(NATIVE_FILE_DIALOG_PLATFORM_SOURCE
            "nativeFileDialog/src/nfd_win.cpp")
elseif(MATH_ANIMATION_OS_OSX)
    set(NATIVE_FILE_DIALOG_PLATFORM_SOURCE
            "nativeFileDialog/src/nfd_cocoa.m")
endif()

add_library(NativeFileDialog ${NATIVE_FILE_DIALOG_SOURCE} ${NATIVE_FILE_DIALOG_PLATFORM_SOURCE})
target_include_directories(NativeFileDialog PRIVATE ${NATIVE_FILE_DIALOG_PRIVATE_INCLUDE_DIR})
target_include_directories(NativeFileDialog PUBLIC ${NATIVE_FILE_DIALOG_INCLUDE_DIR})

if(NATIVE_FILE_DIALOG_USE_GTK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    target_link_libraries(NativeFileDialog PRIVATE ${GTK3_LIBRARIES})
    target_link_directories(NativeFileDialog PRIVATE ${GTK3_LIBRARY_DIRS})
    target_include_directories(NativeFileDialog PRIVATE ${GTK3_INCLUDE_DIRS})
endif()
