include(${CMAKE_MODULE_PATH}/DefaultProject.cmake)

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
