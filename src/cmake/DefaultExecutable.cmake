cmake_minimum_required(VERSION 2.8)

# Extract project name from folder name
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

# Pipe path to ImGui fonts
add_definitions(-DIMGUI_FONTS_PATH="${EXTERNALS_PATH}/imgui/extra_fonts")

# Libraries of this framework
include_directories(
    ${LIBRARIES_PATH}
)

# Collect own source code
file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADER *.h)

# Create executable
add_executable(${ProjectId} ${SOURCES} ${HEADER} ${IM_GUI})

# Link with other libraries
target_link_libraries(
    ${ProjectId}
    ${ALL_LIBRARIES}
)
