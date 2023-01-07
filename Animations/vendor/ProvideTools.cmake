# Provides a Unix tool.
#
# This first attempts to use the system provided tools. If a tool is
# not available, a local busybox executable is used to provide the
# referenced tool. That also means that only tools included in Busybox
# can be automatically provided.
#
# Usage: provide_tool(MY_AWK_PROGRAM awk)
# => MY_AWK_PROGRAM now contains the path to an awk executable
function(provide_tool _OUT_VAR _TOOL)
    # Meta variable
    set(_CACHED_TOOL_VAR "_CACHED_BUSYBOX_${_TOOL}")

    if(DEFINED ${_CACHED_TOOL_VAR})
        # We already have a cached variant available
        set(${_OUT_VAR} ${${_CACHED_TOOL_VAR}} PARENT_SCOPE)
        return()
    endif()

    # First try to find the tool on the path.
    find_path(_FOUND_TOOL NAMES ${_TOOL} ${_TOOL}.exe)

    if (_FOUND_TOOL)
        # We found the tool provided by the system.
        message(STATUS "Found ${_TOOL} at ${_FOUND_TOOL}")

        # Cache and return, we are done
        set(${_OUT_VAR} ${_FOUND_TOOL} PARENT_SCOPE)
        set(${_CACHED_TOOL_VAR} ${_FOUND_TOOL} CACHE STRING "Cached path provided ${_TOOL}" FORCE)
        return()
    endif()

    if(WIN32)
        # Check if we already have some Busybox available
        if(NOT DEFINED PROVIDED_BUSYBOX_EXECUTABLE)
            # Not busybox yet, maybe its on the path
            find_path(_FOUND_BUSYBOX NAMES busybox busybox.exe)

            if(_FOUND_BUSYBOX)
                # Found busybox on the path, we'll use that
                set(PROVIDED_BUSYBOX_EXECUTABLE "${_FOUND_BUSYBOX}" CACHE STRING "Cached path provided busybox" FORCE)
                message(STATUS "Using Busybox at ${PROVIDED_BUSYBOX_EXECUTABLE}")
            else()
                # No busybox on the path, download one!
                set(PROVIDED_BUSYBOX_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/busybox.exe" CACHE STRING "Cached CMake provided busybox" FORCE)
                message(STATUS "Could not find a busybox installation, providing one using busybox-w32!")
                file(DOWNLOAD "https://frippery.org/files/busybox/busybox.exe" "${PROVIDED_BUSYBOX_EXECUTABLE}")
            endif()
        endif()

        message(STATUS "Providing ${_TOOL} using busybox")

        # We now need to copy the busybox executable and rename it to the name
        # of the tool we want it to provide.
        #
        # Busybox will automagically detect it's own executable name and adjust
        # accordingly.
        set(_TOOL_BUSYBOX_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/${_TOOL}.exe")
        file(COPY_FILE "${PROVIDED_BUSYBOX_EXECUTABLE}" "${_TOOL_BUSYBOX_EXECUTABLE}")

        # Cache it and set the output variable
        set(${_OUT_VAR} ${_TOOL_BUSYBOX_EXECUTABLE} PARENT_SCOPE)
        set(${_CACHED_TOOL_VAR} ${_TOOL_BUSYBOX_EXECUTABLE} CACHE STRING "Cached busybox provided ${_TOOL}" FORCE)
    else()
        # For now automatic downloading is only supported on windows.
        #
        # Unix system will probably always provide the requested
        # tools anyway, so it's unlikely this will ever be needed on some
        # platform other than windows.
        message(SEND_ERROR "Tool ${_TOOL} not found, please install it on your system!")
    endif()
endfunction()

# Provides the path to a Unix tool.
#
# Usage: provide_tool_path(MY_AWK_PROGRAM_DIRECTORY awk)
# => MY_AWK_PROGRAM_DIRECTORY now contains the directory path of an awk executable
function(provide_tool_path _OUT_VAR _TOOL)
    # Get the full path including the executable
    provide_tool(_PROVIDED_TOOL ${_TOOL})

    # Strip the filename
    get_filename_component(_TOOL_DIRECTORY "${_PROVIDED_TOOL}" DIRECTORY)
    set(${_OUT_VAR} ${_TOOL_DIRECTORY} PARENT_SCOPE)
endfunction()

# Appends a list of paths to a variable in PATH environment style.
#
# Usage: append_directory_environment(MY_ENVIRONMENT_PATH "/a/b/c" "/x/y/z")
# (windows) => MY_ENVIRONMENT_PATH is now "/a/b/c;/x/y/z"
# (unix)    => MY_ENVIRONMENT_PATH is now "/a/b/c:/x/y/z"
#
# The given variable is not overwritten, but appended instead. This can be used
# to append directories to an existing PATH environment.
function(append_directory_environment _OUT_VAR)
    foreach(_EXTRA IN LISTS ARGN)
        if(WIN32)
            set(${_OUT_VAR} "${${_OUT_VAR}};${_EXTRA}")
        else()
            set(${_OUT_VAR} "${${_OUT_VAR}}:${_EXTRA}")
        endif()
    endforeach()
endfunction()

# Provides a list of paths of tools in PATH environment style. The PATH environment
# variable is prepended automatically!
#
# Additionally the following variables are included in the path:
# - CMAKE_PROGRAM_PATH
# - CMAKE_SYSTEM_PROGRAM_PATH
# - CMAKE_PREFIX
#
# This function can be used to hand-off build functionality to an external build system,
# which requires an environment set-up by CMake.
#
# Usage: provide_tools_path_environment(MY_TOOLS_ENVIRONMENT_PATH sh awk grep)
# (windows) => MY_TOOLS_ENVIRONMENT_PATH is now "<... existing $PATH>;/tools/sh.exe;/abc/awk.exe;/xyz/grep.exe"
# (unix     => MY_TOOLS_ENVIRONMENT_PATH is now "<... existing $PATH>:/usr/bin/sh:/usr/bin/awk:/usr/local/bin/grep"
#
# The given variable is overwritten.
function(provide_tools_path_environment _OUT_VAR)
    # We start out with the raw $PATH
    cmake_path(SET _COLLECTED_PATH "$ENV{PATH}")

    # Take into account CMake's path extensions
    foreach(_PATH IN LISTS CMAKE_PROGRAM_PATH CMAKE_SYSTEM_PROGRAM_PATH CMAKE_PREFIX)
        append_directory_environment(_COLLECTED_PATH "${_PATH}")
    endforeach()

    # For each tool provide its directory and append it to the environment
    foreach(_TOOL IN LISTS ARGN)
        provide_tool_path(_TOOL_PATH ${_TOOL})
        append_directory_environment(_COLLECTED_PATH "${_TOOL_PATH}")
    endforeach()

    # Set the collected path
    set(${_OUT_VAR} "${_COLLECTED_PATH}" PARENT_SCOPE)
endfunction()
