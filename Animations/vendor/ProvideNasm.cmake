# Provides a NASM assembler for assembling x86-64.
#
# This first attempts to use the system NASM. If not available,
# it will attempt to download one.

# Usage: provide_nasm(MY_NASM_PROGRAM)
# => MY_NASM_PROGRAM now contains the path to a NASM executable
function(provide_nasm _OUT_VAR)
    if(DEFINED _CACHED_NASM)
        # We already have a cached variant available
        set(${_OUT_VAR} ${_CACHED_NASM} PARENT_SCOPE)
        return()
    endif()

    # First try to find NASM on the path.
    find_path(_NASM NAMES nasm nasm.exe)

    if (_NASM)
        # We found a NASM system installation.
        message(STATUS "Found nasm at ${_NASM}")

        # Cache and return, we are done
        set(${_OUT_VAR} ${_NASM} PARENT_SCOPE)
        set(_CACHED_NASM ${_NASM} CACHE STRING "Cached nasm" FORCE)
        return()
    endif()


    if(WIN32)
        # On windows it is quite normal for NASM to not be present.
        #
        # We attempt to download the official NASM binary. It comes
        # packaged as a ZIP, so we need to extract it after downloading.
        set(_NASM_VERSION "2.16.01")
        set(_NASM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/nasm")
        set(_NASM_DOWNLOAD_TARGET_ZIP "${_NASM_BINARY_DIR}/nasm.zip")
        set(_NASM_EXTRACT_TARGET "${_NASM_BINARY_DIR}/bin")

        # Download and extract
        message(STATUS "Could not find nasm, downloading it!")
        file(DOWNLOAD "https://www.nasm.us/pub/nasm/releasebuilds/${_NASM_VERSION}/win64/nasm-${_NASM_VERSION}-win64.zip" "${_NASM_DOWNLOAD_TARGET_ZIP}")
        file(ARCHIVE_EXTRACT
                INPUT "${_NASM_DOWNLOAD_TARGET_ZIP}"
                DESTINATION "${_NASM_BINARY_DIR}"
        )

        # Download successful
        set(_NASM_BINARY "${_NASM_BINARY_DIR}/nasm-${_NASM_VERSION}/nasm.exe")

        # Cache it and set the output variable
        set(${_OUT_VAR} ${_NASM_BINARY} PARENT_SCOPE)
        set(_CACHED_NASM ${_NASM_BINARY} CACHE STRING "Cached nasm" FORCE)
    else()
        # For now automatic downloading is only supported on windows.
        #
        # NASM can usually be installed using the system repositories.
        message(SEND_ERROR "No nasm found and unable to automatically install one, add nasm to PATH!")
    endif()
endfunction()
