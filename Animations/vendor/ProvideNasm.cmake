function(provide_nasm _OUT_VAR)
    if(DEFINED _CACHED_NASM)
        set(${_OUT_VAR} ${_CACHED_NASM} PARENT_SCOPE)
        return()
    endif()

    find_path(_NASM NAMES nasm nasm.exe)

    if (_NASM)
        message(STATUS "Found nasm at ${_NASM}")

        set(${_OUT_VAR} ${_NASM} PARENT_SCOPE)
        set(_CACHED_NASM ${_NASM} CACHE STRING "Cached nasm" FORCE)
        return()
    endif()


    if(WIN32)
        set(_NASM_VERSION "2.16.01")
        set(_NASM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/nasm")
        set(_NASM_DOWNLOAD_TARGET_ZIP "${_NASM_BINARY_DIR}/nasm.zip")
        set(_NASM_EXTRACT_TARGET "${_NASM_BINARY_DIR}/bin")

        message(STATUS "Could not find nasm, downloading it!")
        file(DOWNLOAD "https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win64/nasm-2.16.01-win64.zip" "${_NASM_DOWNLOAD_TARGET_ZIP}")
        file(ARCHIVE_EXTRACT
                INPUT "${_NASM_DOWNLOAD_TARGET_ZIP}"
                DESTINATION "${_NASM_BINARY_DIR}"
        )

        set(_NASM_BINARY "${_NASM_BINARY_DIR}/nasm-${_NASM_VERSION}/nasm.exe")

        set(${_OUT_VAR} ${_NASM_BINARY} PARENT_SCOPE)
        set(_CACHED_NASM ${_NASM_BINARY} CACHE STRING "Cached nasm" FORCE)
    else()
        message(SEND_ERROR "No nasm found and unable to automatically install one, add nasm to PATH!")
    endif()
endfunction()
