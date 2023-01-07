# FFMPEG build support

set(FFMPEG_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_INSTALL_DIR}/include")

set(FFMPEG_LIBRARY_NAMES avcodec avdevice avfilter avformat avutil swresample swscale)
set(FFMPEG_LIBRARY_LOCATIONS)

macro(_get_ffmpeg_library_location _OUT_VAR _FFMPEG_LIBRARY_NAME)
    # TODO: This might be different on Windows
    set(${_OUT_VAR} "${FFMPEG_INSTALL_DIR}/lib/lib${_FFMPEG_LIBRARY_NAME}.a")
endmacro()

foreach(_FFMPEG_LIBRARY_NAME IN LISTS FFMPEG_LIBRARY_NAMES)
    _get_ffmpeg_library_location(FFMPEG_LIBRARY_LOCATION ${_FFMPEG_LIBRARY_NAME})
    set(FFMPEG_LIBRARY_LOCATIONS ${FFMPEG_LIBRARY_LOCATIONS} ${FFMPEG_LIBRARY_LOCATION})
endforeach()

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    message(STATUS "Building with Unix Makefiles, using \"${CMAKE_MAKE_PROGRAM}\" for FFMPEG")
    set(FFMPEG_BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}")
else()
    include(ProcessorCount)
    ProcessorCount(FFMPEG_PARALLEL_COUNT)

    include(ProvideMake.cmake)
    provide_gnu_make(FFMPEG_MAKE)

    set(FFMPEG_BUILD_COMMAND "${FFMPEG_MAKE} -j${FFMPEG_PARALLEL_COUNT}")
    message(STATUS "Building with another generator, auto adjusting FFMPEG build command to \"${FFMPEG_BUILD_COMMAND}\"")
endif()

include(ProvideTools.cmake)
provide_tool(FFMPEG_POSIX_SHELL sh)
provide_tools_path_environment(FFMPEG_BUILD_ENV_PATH sh awk printf rm mkdir touch)
append_directory_environment(FFMPEG_BUILD_ENV_PATH
        "${CMAKE_CXX_COMPILER}"
        "${CMAKE_C_COMPILER}"
        "${CMAKE_C_LINKER_LAUNCHER}"
        "${CMAKE_AR}")

if(MSVC)
    include(ProvideNasm.cmake)
    provide_nasm(FFMPEG_NASM)

    set(FFMPEG_CONFIGURE_OPTIONS "--toolchain=msvc --x86asmexe=\"${FFMPEG_NASM}\" --target-os=win64 --arch=x86_64")
endif()

set(FFMPEG_CONFIGURE_PROXY "${CMAKE_CURRENT_BINARY_DIR}/configure-ffmpeg.sh")
file(WRITE "${FFMPEG_CONFIGURE_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "exec ${CMAKE_CURRENT_LIST_DIR}/ffmpeg/configure --prefix=\"${FFMPEG_INSTALL_DIR}\" \\
                --disable-programs \\
                --disable-doc \\
                --enable-pic \\
                " "${FFMPEG_CONFIGURE_OPTIONS}"
)

set(FFMPEG_BUILD_PROXY "${CMAKE_CURRENT_BINARY_DIR}/build-ffmpeg.sh")
file(WRITE "${FFMPEG_BUILD_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "export SHELL=sh\n"
        "exec ${FFMPEG_BUILD_COMMAND}"
)

set(FFMPEG_INSTALL_PROXY "${CMAKE_CURRENT_BINARY_DIR}/install-ffmpeg.sh")
file(WRITE "${FFMPEG_INSTALL_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "export PREFIX=\"${FFMPEG_INSTALL_DIR}\"\n"
        "exec ${FFMPEG_MAKE} install"
)

file(MAKE_DIRECTORY "${FFMPEG_BUILD_DIR}")
file(COPY "${FFMPEG_POSIX_SHELL}" DESTINATION "${FFMPEG_BUILD_DIR}")

file(MAKE_DIRECTORY "${FFMPEG_INCLUDE_DIR}")

ExternalProject_Add(
        ffmpeg
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/ffmpeg"
        BINARY_DIR "${FFMPEG_BUILD_DIR}"
        CONFIGURE_COMMAND "${FFMPEG_POSIX_SHELL}" "${FFMPEG_CONFIGURE_PROXY}"
        BUILD_COMMAND "${FFMPEG_POSIX_SHELL}" ${FFMPEG_BUILD_PROXY}
        BUILD_BYPRODUCTS ${FFMPEG_LIBRARY_LOCATIONS} "${FFMPEG_INCLUDE_DIR}"
        INSTALL_COMMAND "${FFMPEG_POSIX_SHELL}" "${FFMPEG_INSTALL_PROXY}"
)

function(_add_ffmpeg_library _FFMPEG_LIBRARY_NAME)
    set(_FFMPEG_TARGET_NAME "ffmpeg::${_FFMPEG_LIBRARY_NAME}")
    _get_ffmpeg_library_location(_FFMPEG_LIBRARY_LOCATION ${_FFMPEG_LIBRARY_NAME})

    message(STATUS "Adding ffmpeg library ${_FFMPEG_TARGET_NAME}")

    add_library("${_FFMPEG_TARGET_NAME}" STATIC IMPORTED GLOBAL)
    set_target_properties("${_FFMPEG_TARGET_NAME}" PROPERTIES IMPORTED_LOCATION "${_FFMPEG_LIBRARY_LOCATION}")
    target_include_directories("${_FFMPEG_TARGET_NAME}" INTERFACE "${FFMPEG_INCLUDE_DIR}")
    add_dependencies("${_FFMPEG_TARGET_NAME}" ffmpeg)
endfunction()

foreach(_FFMPEG_LIBRARY_NAME IN LISTS FFMPEG_LIBRARY_NAMES)
    _add_ffmpeg_library(${_FFMPEG_LIBRARY_NAME})
endforeach()

# Some ffmpeg libraries required additional dependencies
if(MATH_ANIMATION_OS_LINUX)
    find_package(X11 REQUIRED)
    find_package(LibLZMA REQUIRED)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(VA_API libva libva-drm libva-x11 vdpau REQUIRED)

    target_link_libraries(ffmpeg::avutil INTERFACE ${X11_LIBRARIES})
    target_link_libraries(ffmpeg::avcodec INTERFACE LibLZMA::LibLZMA ${VA_API_LIBRARIES})
elseif(MATH_ANIMATION_OS_WINDOWS)
    target_link_libraries(ffmpeg::avutil INTERFACE Bcrypt)
    target_link_libraries(ffmpeg::avformat INTERFACE Ws2_32 Secur32)
    target_link_libraries(ffmpeg::avcodec INTERFACE Strmiids Mfuuid)
endif()

target_link_libraries(ffmpeg::avformat INTERFACE ffmpeg::avcodec)

