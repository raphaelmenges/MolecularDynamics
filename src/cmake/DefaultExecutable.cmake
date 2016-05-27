cmake_minimum_required(VERSION 2.8)
if(CMAKE_VERSION VERSION_GREATER "2.8.11")
    CMAKE_POLICY(SET CMP0022 OLD)
endif()

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# ImGui
message(STATUS ImGui)
set(IM_GUI_DIR ${EXTERNALS_PATH}/imgui)
file(GLOB IM_GUI
    ${IM_GUI_DIR}/imgui.cpp
    ${IM_GUI_DIR}/imgui_draw.cpp
    ${IM_GUI_DIR}/examples/opengl3_example/imgui_impl_glfw_gl3.cpp)
include_directories(${IM_GUI_DIR})

# Other libraries
include_directories(
    ${LIBRARIES_PATH}
)

# Collect own soruce code
file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADER *.h)

add_executable(${ProjectId} ${SOURCES} ${HEADER} ${IM_GUI})

target_link_libraries(
    ${ProjectId}
    ${ALL_LIBRARIES}
)