# FFMPEG build support

# FFMPEG is a bit hard to build on windows, this CMake script
# automates it as much as possible.
#
# More specifically FFMPEG expects a Unix like environment. The
# configure script requires a posix shell and the generated Makefiles
# assume a Unix like toolset. All of those utilities can be automatically
# provided on Windows (if an internet connection is available).
#
# All required Unix utilities are cleanly installed in the build directory
# and do not touch anything outside of it. This should ensure builds are
# reproducible and don't mess with the system.

# These directories will be the final directories in the build tree
# which contain the source and installed headers/libraries
set(FFMPEG_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_INSTALL_DIR}/include")

# Create the build directory already, we will put some
# helpers into it.
file(MAKE_DIRECTORY "${FFMPEG_BUILD_DIR}")

# Here we list all libraries we use with FFMPEG/FFMPEG provides
set(FFMPEG_LIBRARY_NAMES avcodec avdevice avfilter avformat avutil swresample swscale)
set(FFMPEG_LIBRARY_LOCATIONS)

# Helper macro to get the final object archive location of a library built by ffmpeg.
#
# Usage: _get_ffmpeg_library_location(MY_LOCATION_VARIABLE avcodec)
macro(_get_ffmpeg_library_location _OUT_VAR _FFMPEG_LIBRARY_NAME)
    set(${_OUT_VAR} "${FFMPEG_INSTALL_DIR}/lib/lib${_FFMPEG_LIBRARY_NAME}.a")
endmacro()

# Collect the final locations of the built libraries into a list,
# we need this so we can tell CMake (and the build systems) where to
# look for files.
foreach(_FFMPEG_LIBRARY_NAME IN LISTS FFMPEG_LIBRARY_NAMES)
    _get_ffmpeg_library_location(FFMPEG_LIBRARY_LOCATION ${_FFMPEG_LIBRARY_NAME})
    set(FFMPEG_LIBRARY_LOCATIONS ${FFMPEG_LIBRARY_LOCATIONS} ${FFMPEG_LIBRARY_LOCATION})
endforeach()

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # If we are generating for Unix Makefiles already, we have a Unix Makefile compatible
    # build system. If so, we can just pass this through, as FFMPEG uses Unix Makefile's.
    message(STATUS "Building with Unix Makefiles, using \"${CMAKE_MAKE_PROGRAM}\" for FFMPEG")

    set(FFMPEG_MAKE "${CMAKE_MAKE_PROGRAM}")
    set(FFMPEG_BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}")
else()
    # We are generating for something which isn't Unix Make. This could be Ninja, MSBuild or really
    # anything. In this case we need some compatibility options. More specifically we have to find
    # (or create) a compatible Unix/Gnu make program.

    # Enable parallel builds (currently always uses all cores, the build system may not support a jobserver)
    include(ProcessorCount)
    ProcessorCount(FFMPEG_PARALLEL_COUNT)

    # Get access to a GNU Make.
    #
    # This will either use the system make if available or download one (if possible).
    include(ProvideMake.cmake)
    provide_gnu_make(FFMPEG_MAKE)

    # Our build command will now be "/path/to/make -j${processor-count}"
    set(FFMPEG_BUILD_COMMAND "${FFMPEG_MAKE} -j${FFMPEG_PARALLEL_COUNT}")
    message(STATUS "Building with another generator, auto adjusting FFMPEG build command to \"${FFMPEG_BUILD_COMMAND}\"")
endif()

# We need some Unix tools for building FFMPEG.
#
# This is a quite complex process, see ProvideTools.cmake for more info.
include(ProvideTools.cmake)

# We need a posix shell for building, obtain one
provide_tool(FFMPEG_POSIX_SHELL sh)

# The FFMPEG Makefiles rely on some Unix tools.
#
# We collect all the required tools into a path environment and then
# also append the selected compilers. This gives us a $PATH environment variable
# which includes everything we need to for building FFMPEG.
provide_tools_path_environment(FFMPEG_BUILD_ENV_PATH
        sh
        awk
        printf
        rm
        mkdir
        touch
)
append_directory_environment(FFMPEG_BUILD_ENV_PATH
        "${CMAKE_CXX_COMPILER}"
        "${CMAKE_C_COMPILER}"
        "${CMAKE_C_LINKER_LAUNCHER}"
        "${CMAKE_AR}")

# FFMPEG requires NASM
include(ProvideNasm.cmake)
provide_nasm(FFMPEG_NASM)

if(MSVC)
    # We also need some additional configure options for building with MSVC
    set(FFMPEG_CONFIGURE_OPTIONS "--toolchain=msvc --target-os=win64 --arch=x86_64")
endif()

# Generate a shell script which can be run by a posix shell and prepares the environment
# for configuration
set(FFMPEG_CONFIGURE_PROXY "${FFMPEG_BUILD_DIR}/configure-ffmpeg.sh")
file(WRITE "${FFMPEG_CONFIGURE_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "exec ${CMAKE_CURRENT_LIST_DIR}/ffmpeg/configure --prefix=\"${FFMPEG_INSTALL_DIR}\" \\
                --disable-programs \\
                --disable-doc \\
                --enable-pic \\
                --x86asmexe=\"${FFMPEG_NASM}\" \\
                " "${FFMPEG_CONFIGURE_OPTIONS}"
)

# Same as above, but for building
set(FFMPEG_BUILD_PROXY "${FFMPEG_BUILD_DIR}/build-ffmpeg.sh")
file(WRITE "${FFMPEG_BUILD_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "export SHELL=sh\n"
        "exec ${FFMPEG_BUILD_COMMAND}"
)

# Same as above, but for installing
set(FFMPEG_INSTALL_PROXY "${FFMPEG_BUILD_DIR}/install-ffmpeg.sh")
file(WRITE "${FFMPEG_INSTALL_PROXY}"
        "export PATH=\"${FFMPEG_BUILD_ENV_PATH}\"\n"
        "export PREFIX=\"${FFMPEG_INSTALL_DIR}\"\n"
        "exec ${FFMPEG_MAKE} install"
)

if(MSVC)
    # This is a GNU make oddity on windows. If the shell executable is not in the
    # build directory, it will use cmd.exe which results in everything breaking,
    # because the required shell commands are only available in a posix shell.
    #
    # This also requires the SHELL environment variable to be set to "sh", which
    # the build scripts above take care of.
    file(COPY "${FFMPEG_POSIX_SHELL}" DESTINATION "${FFMPEG_BUILD_DIR}")
endif()

# Some generators else refuse to add the library targets
file(MAKE_DIRECTORY "${FFMPEG_INCLUDE_DIR}")

# After all this setup we can now finally add the FFMPEG project itself
ExternalProject_Add(
        ffmpeg
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/ffmpeg"
        BINARY_DIR "${FFMPEG_BUILD_DIR}"
        CONFIGURE_COMMAND "${FFMPEG_POSIX_SHELL}" "${FFMPEG_CONFIGURE_PROXY}"
        BUILD_COMMAND "${FFMPEG_POSIX_SHELL}" ${FFMPEG_BUILD_PROXY}
        BUILD_BYPRODUCTS ${FFMPEG_LIBRARY_LOCATIONS} "${FFMPEG_INCLUDE_DIR}"
        INSTALL_COMMAND "${FFMPEG_POSIX_SHELL}" "${FFMPEG_INSTALL_PROXY}"
)

# Helper function to perform some functionality for adding an FFMPEG library target.
#
# Usage: _add_ffmpeg_library(avcodec)
# => This creates a CMake target ffmpeg::avcodec, which can be used to link against
# libavcodec.
function(_add_ffmpeg_library _FFMPEG_LIBRARY_NAME)
    # Find the location of the generated library file
    set(_FFMPEG_TARGET_NAME "ffmpeg::${_FFMPEG_LIBRARY_NAME}")
    _get_ffmpeg_library_location(_FFMPEG_LIBRARY_LOCATION ${_FFMPEG_LIBRARY_NAME})

    message(STATUS "Adding ffmpeg library ${_FFMPEG_TARGET_NAME}")

    # Add the target and configure its CMake properties
    add_library("${_FFMPEG_TARGET_NAME}" STATIC IMPORTED GLOBAL)
    set_target_properties("${_FFMPEG_TARGET_NAME}" PROPERTIES IMPORTED_LOCATION "${_FFMPEG_LIBRARY_LOCATION}")
    target_include_directories("${_FFMPEG_TARGET_NAME}" INTERFACE "${FFMPEG_INCLUDE_DIR}")

    # We first need to build the external FFMPEG project in order for this to become valid
    add_dependencies("${_FFMPEG_TARGET_NAME}" ffmpeg)
endfunction()

# Loop over all FFMPEG libraries and create their respective targets.
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
    target_link_libraries(ffmpeg::avcodec INTERFACE Strmiids Mfuuid Mfplat)
endif()

# There probably should be more here...
target_link_libraries(ffmpeg::avformat INTERFACE ffmpeg::avcodec)
