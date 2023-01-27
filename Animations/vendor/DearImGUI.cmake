# Dear ImGUI build support
# ImGUI itself
set(DEAR_IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/dearimgui")

set(DEAR_IMGUI_SOURCE
        "dearimgui/imgui.cpp"
        "dearimgui/imgui_demo.cpp"
        "dearimgui/imgui_draw.cpp"
        "dearimgui/imgui_tables.cpp"
        "dearimgui/imgui_widgets.cpp")

add_library(DearImGUI ${DEAR_IMGUI_SOURCE})
target_include_directories(DearImGUI PUBLIC ${DEAR_IMGUI_INCLUDE_DIR})

# GLFW OpenGL3 backend

set(DEAR_IMGUI_GLFW_OGL3_BACKEND_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/dearimgui/backends")

set(DEAR_IMGUI_GLFW_OGL3_BACKEND_SOURCE
        "dearimgui/backends/imgui_impl_opengl3.cpp"
        "dearimgui/backends/imgui_impl_glfw.cpp")

add_library(DearImGUIGlfwOgl3Backend ${DEAR_IMGUI_GLFW_OGL3_BACKEND_SOURCE})
target_include_directories(DearImGUIGlfwOgl3Backend PUBLIC ${DEAR_IMGUI_GLFW_OGL3_BACKEND_INCLUDE_DIR})
target_link_libraries(DearImGUIGlfwOgl3Backend PUBLIC glfw DearImGUI)

if(DEFINED "DEAR_IMGUI_USER_CONFIG")
    message(STATUS "Using ImGUI config header at ${DEAR_IMGUI_USER_CONFIG}")
    target_compile_definitions(DearImGUI PUBLIC IMGUI_USER_CONFIG="${DEAR_IMGUI_USER_CONFIG}")
    target_compile_definitions(DearImGUIGlfwOgl3Backend PUBLIC IMGUI_USER_CONFIG="${DEAR_IMGUI_USER_CONFIG}")
endif()


if(DEFINED "DEAR_IMGUI_EXTRA_INCLUDE_DIRS")
    target_include_directories(DearImGUI PRIVATE ${DEAR_IMGUI_EXTRA_INCLUDE_DIRS})
    target_include_directories(DearImGUIGlfwOgl3Backend PRIVATE ${DEAR_IMGUI_EXTRA_INCLUDE_DIRS})
endif()

