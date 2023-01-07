# Provides a GNU Make program for use with Unix Makefile's.
#
# This first attempts to use the system Make program. If not available,
# it will attempt to download one.
#
# Usage: provide_gnu_make(MY_MAKE_PROGRAM)
# => MY_MAKE_PROGRAM now contains the path to a make executable
function(provide_gnu_make _OUT_VAR)
    if(DEFINED _CACHED_GNU_MAKE)
        # We already have a cached variant available
        set(${_OUT_VAR} ${_CACHED_GNU_MAKE} PARENT_SCOPE)
        return()
    endif()

    # First try to find make or gmake on the path.
    #
    # We prefer gmake, as this is usually is the GNU compatible one
    # on systems where the normal make is not GNU compatible.
    find_path(_GNU_MAKE NAMES gmake make gmake.exe make.exe)

    if (_GNU_MAKE)
        # We found a GNU make system installation.
        message(STATUS "Found GNU make at ${_GNU_MAKE}")

        # Cache and return, we are done
        set(${_OUT_VAR} ${_GNU_MAKE} PARENT_SCOPE)
        set(_CACHED_GNU_MAKE ${_GNU_MAKE} CACHE STRING "Cached GNU make" FORCE)
        return()
    endif()


    if(WIN32)
        # On windows it is quite normal for make to not be present.
        #
        # We attempt to download one from the gnumake-windows project and
        # store it in the build directory.
        set(_GNU_MAKE_DOWNLOAD_TARGET "${CMAKE_CURRENT_BINARY_DIR}/make.exe")

        message(STATUS "Could not find GNU make, downloading gnumake-windows to provide one!")
        file(DOWNLOAD "https://github.com/mbuilov/gnumake-windows/raw/master/gnumake-4.3-x64.exe" "${_GNU_MAKE_DOWNLOAD_TARGET}")

        # Download successful, cache it and set the output variable
        set(${_OUT_VAR} ${_GNU_MAKE_DOWNLOAD_TARGET} PARENT_SCOPE)
        set(_CACHED_GNU_MAKE ${_GNU_MAKE_DOWNLOAD_TARGET} CACHE STRING "Cached GNU make" FORCE)
    else()
        # For now automatic downloading is only supported on windows.
        #
        # Virtually every other system will provide a GNU compatible make anyway.
        message(SEND_ERROR "No GNU Make found and unable to automatically install one, add make/gmake to PATH!")
    endif()
endfunction()
