function(provide_gnu_make _OUT_VAR)
    if(DEFINED _CACHED_GNU_MAKE)
        set(${_OUT_VAR} ${_CACHED_GNU_MAKE} PARENT_SCOPE)
        return()
    endif()

    find_path(_GNU_MAKE NAMES make gmake make.exe gmake.exe)

    if (_GNU_MAKE)
        message(STATUS "Found GNU make at ${_GNU_MAKE}")

        set(${_OUT_VAR} ${_GNU_MAKE} PARENT_SCOPE)
        set(_CACHED_GNU_MAKE ${_GNU_MAKE} CACHE STRING "Cached GNU make" FORCE)
        return()
    endif()


    if(WIN32)
        set(_GNU_MAKE_DOWNLOAD_TARGET "${CMAKE_CURRENT_BINARY_DIR}/make.exe")

        message(STATUS "Could not find GNU make, downloading gnumake-windows to provide one!")
        file(DOWNLOAD "https://github.com/mbuilov/gnumake-windows/raw/master/gnumake-4.3-x64.exe" "${_GNU_MAKE_DOWNLOAD_TARGET}")

        set(${_OUT_VAR} ${_GNU_MAKE_DOWNLOAD_TARGET} PARENT_SCOPE)
        set(_CACHED_GNU_MAKE ${_GNU_MAKE_DOWNLOAD_TARGET} CACHE STRING "Cached GNU make" FORCE)
    else()
        message(SEND_ERROR "No GNU Make found and unable to automatically install one, add make/gmake to PATH!")
    endif()
endfunction()
