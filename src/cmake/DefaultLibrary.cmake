include(${CMAKE_MODULE_PATH}/DefaultProject.cmake)

# Seems that all libraries get collected and are linked
# against everything. Because CMake tells about not linking
# to itself, set policy to old behavior to shut down any
# warnings about it
cmake_policy(SET CMP0038 OLD)

# Extract project name from folder name
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# Libraries of this framework
include_directories(${LIBRARIES_PATH})

# Collect source code
file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADER *.h)

# Create library
add_library(${ProjectId} ${SOURCES} ${HEADER})

# Link with other libraries
target_link_libraries(
    ${ProjectId}
    ${ALL_LIBRARIES}
)
