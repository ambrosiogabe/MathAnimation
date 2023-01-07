# This glad library has been generated using gen.glad.sh
# Permalink: https://gen.glad.sh/#generator=c&api=gl%3D4.4&profile=gl%3Dcompatibility%2Cgles1%3Dcommon&extensions=GL_KHR_debug
#
# To update replace the src and include directories

set(GLAD_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(GLAD_SOURCE
        src/gl.c)

add_library(glad ${GLAD_SOURCE})
target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIR})
