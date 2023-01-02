# FFMPEG build support

set(FFMPEG_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix")

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

    set(FFMPEG_BUILD_COMMAND make -j${FFMPEG_PARALLEL_COUNT})
    message(STATUS "Building with another generator, auto adjusting FFMPEG build command to \"${FFMPEG_BUILD_COMMAND}\"")
endif()

ExternalProject_Add(
        ffmpeg
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/ffmpeg"
        BINARY_DIR "${FFMPEG_BUILD_DIR}"
        CONFIGURE_COMMAND "${CMAKE_CURRENT_LIST_DIR}/ffmpeg/configure"
            --prefix="${FFMPEG_INSTALL_DIR}"
            --disable-programs
            --disable-doc
            --enable-pic
        BUILD_COMMAND ${FFMPEG_BUILD_COMMAND}
        BUILD_BYPRODUCTS ${FFMPEG_LIBRARY_LOCATIONS}
)

function(_add_ffmpeg_library _FFMPEG_LIBRARY_NAME)
    set(_FFMPEG_TARGET_NAME "ffmpeg::${_FFMPEG_LIBRARY_NAME}")
    _get_ffmpeg_library_location(_FFMPEG_LIBRARY_LOCATION ${_FFMPEG_LIBRARY_NAME})
    set(_FFMPEG_LIBRARY_INCLUDE_DIR "${FFMPEG_INSTALL_DIR/bin/include/${_FFMPEG_LIBRARY_NAME}}")

    message(STATUS "Adding ffmpeg library ${_FFMPEG_TARGET_NAME}")

    add_library("${_FFMPEG_TARGET_NAME}" STATIC IMPORTED GLOBAL)
    set_target_properties("${_FFMPEG_TARGET_NAME}" PROPERTIES IMPORTED_LOCATION "${_FFMPEG_LIBRARY_LOCATION}")
    target_include_directories("${_FFMPEG_TARGET_NAME}" INTERFACE "${_FFMPEG_LIBRARY_INCLUDE_DIR}")
    add_dependencies("${_FFMPEG_TARGET_NAME}" ffmpeg)
endfunction()

foreach(_FFMPEG_LIBRARY_NAME IN LISTS FFMPEG_LIBRARY_NAMES)
    _add_ffmpeg_library(${_FFMPEG_LIBRARY_NAME})
endforeach()

if(MATH_ANIMATION_OS_LINUX)
    # Some ffmpeg libraries required additional dependencies
    find_package(X11 REQUIRED)
    find_package(LibLZMA REQUIRED)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(VA_API libva libva-drm libva-x11 vdpau REQUIRED)

    target_link_libraries(ffmpeg::avutil INTERFACE ${X11_LIBRARIES})
    target_link_libraries(ffmpeg::avcodec INTERFACE LibLZMA::LibLZMA ${VA_API_LIBRARIES})
    target_link_libraries(ffmpeg::avformat INTERFACE ffmpeg::avcodec)
endif()

# TODO: These libraries need to be linked on Windows with ffmpeg:
#     Ws2_32
#    Secur32
#    Bcrypt
#    Mfuuid
#    Strmiids
