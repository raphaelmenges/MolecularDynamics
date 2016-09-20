cmake_minimum_required(VERSION 2.8)

# Name of framework
project(OpenGLFramework)

# Prepare path finding
set(MINICONDA3_PATH "$ENV{HOME}/miniconda3")

# Set paths
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "Path to custom CMake modules.")
set(EXTERNALS_PATH "${CMAKE_SOURCE_DIR}/../externals" CACHE PATH "Path to external code.")
set(RESOURCES_PATH "${CMAKE_SOURCE_DIR}/../resources" CACHE PATH "Path to resources.")
set(EXECUTABLES_PATH "${CMAKE_SOURCE_DIR}/executables" CACHE PATH "Path to code of executables.")
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/libraries" CACHE PATH "Path to code of libraries.")
set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/shaders" CACHE PATH "Path to code of shaders.")
set(PYTHON_INCLUDE_DIRS "${MINICONDA3_PATH}/include/python3.5m" CACHE PATH "Path to Python include directory.")
set(PYTHON_LIBRARIES "${MINICONDA3_PATH}/lib/libpython3.5m.so" CACHE PATH "Path of Python shared library.")
set(PYTHON_PACKAGES_PATH "${MINICONDA3_PATH}/lib/python3.5/site-packages" CACHE PATH "Path to Python packages.")

# Include cmake macros
include(${CMAKE_MODULE_PATH}/doxygen.cmake)
include(${CMAKE_MODULE_PATH}/macros.cmake)

# Set output paths for libraries
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
GENERATE_SUBDIRS(ALL_LIBRARIES "${LIBRARIES_PATH}" "${PROJECT_BINARY_DIR}/libraries")

# Set output paths for executables
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
GENERATE_SUBDIRS(ALL_EXECUTABLES "${EXECUTABLES_PATH}" "${PROJECT_BINARY_DIR}/executables")

# Add shader path as subdirectory to have it available in project tree
if(EXISTS "${SHADERS_PATH}")
        add_subdirectory("${SHADERS_PATH}")
endif()
