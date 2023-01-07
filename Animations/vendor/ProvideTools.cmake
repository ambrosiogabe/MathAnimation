function(provide_tool _OUT_VAR _TOOL)
    set(_CACHED_TOOL_VAR "_CACHED_BUSYBOX_${_TOOL}")

    if(DEFINED ${_CACHED_TOOL_VAR})
        set(${_OUT_VAR} ${${_CACHED_TOOL_VAR}} PARENT_SCOPE)
        return()
    endif()

    find_path(_FOUND_TOOL NAMES ${_TOOL} ${_TOOL}.exe)

    if (_FOUND_TOOL)
        message(STATUS "Found ${_TOOL} at ${_FOUND_TOOL}")

        set(${_OUT_VAR} ${_FOUND_TOOL} PARENT_SCOPE)
        set(${_CACHED_TOOL_VAR} ${_FOUND_TOOL} CACHE STRING "Cached path provided ${_TOOL}" FORCE)
        return()
    endif()

    if(WIN32)
        if(NOT DEFINED PROVIDED_BUSYBOX_EXECUTABLE)
            find_path(_FOUND_BUSYBOX NAMES busybox busybox.exe)

            if(_FOUND_BUSYBOX)
                set(PROVIDED_BUSYBOX_EXECUTABLE "${_FOUND_BUSYBOX}" CACHE STRING "Cached path provided busybox" FORCE)
                message(STATUS "Using Busybox at ${PROVIDED_BUSYBOX_EXECUTABLE}")
            else()
                set(PROVIDED_BUSYBOX_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/busybox.exe" CACHE STRING "Cached CMake provided busybox" FORCE)
                message(STATUS "Could not find a busybox installation, providing one using busybox-w32!")
                file(DOWNLOAD "https://frippery.org/files/busybox/busybox.exe" "${PROVIDED_BUSYBOX_EXECUTABLE}")
            endif()
        endif()

        message(STATUS "Providing ${_TOOL} using busybox")

        set(_TOOL_BUSYBOX_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/${_TOOL}.exe")
        file(COPY_FILE "${PROVIDED_BUSYBOX_EXECUTABLE}" "${_TOOL_BUSYBOX_EXECUTABLE}")

        set(${_OUT_VAR} ${_TOOL_BUSYBOX_EXECUTABLE} PARENT_SCOPE)
        set(${_CACHED_TOOL_VAR} ${_TOOL_BUSYBOX_EXECUTABLE} CACHE STRING "Cached busybox provided ${_TOOL}" FORCE)
    else()
        message(SEND_ERROR "Tool ${_TOOL} not found, please install it on your system!")
    endif()
endfunction()

function(provide_tool_path _OUT_VAR _TOOL)
    provide_tool(_PROVIDED_TOOL ${_TOOL})

    get_filename_component(_TOOL_DIRECTORY "${_PROVIDED_TOOL}" DIRECTORY)
    set(${_OUT_VAR} ${_TOOL_DIRECTORY} PARENT_SCOPE)
endfunction()

function(append_directory_environment _OUT_VAR)
    foreach(_EXTRA IN LISTS ARGN)
        if(WIN32)
            set(${_OUT_VAR} "${${_OUT_VAR}};${_EXTRA}")
        else()
            set(${_OUT_VAR} "${${_OUT_VAR}}:${_EXTRA}")
        endif()
    endforeach()
endfunction()

function(provide_tools_path_environment _OUT_VAR)
    cmake_path(SET _COLLECTED_PATH "$ENV{PATH}")

    foreach(_TOOL IN LISTS ARGN)
        provide_tool_path(_TOOL_PATH ${_TOOL})
        append_directory_environment(_COLLECTED_PATH "${_TOOL_PATH}")
    endforeach()

    foreach(_PATH IN LISTS CMAKE_PROGRAM_PATH CMAKE_SYSTEM_PROGRAM_PATH CMAKE_PREFIX)
        append_directory_environment(_COLLECTED_PATH "${_PATH}")
    endforeach()

    set(${_OUT_VAR} "${_COLLECTED_PATH}" PARENT_SCOPE)
endfunction()
