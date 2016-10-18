cmake_minimum_required(VERSION 2.8)

# Name of framework
project(OpenGLFramework)

# Prepare path finding
set(MINICONDA3_PATH "$ENV{HOME}/miniconda3")

# Set paths
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "Path to custom CMake modules.")
set(EXTERNALS_PATH "${CMAKE_SOURCE_DIR}/../externals" CACHE PATH "Path to external code.")
set(SUBMODULESS_PATH "${CMAKE_SOURCE_DIR}/../submodules" CACHE PATH "Path to submodules.")
set(RESOURCES_PATH "${CMAKE_SOURCE_DIR}/../resources" CACHE PATH "Path to resources.")
set(EXECUTABLES_PATH "${CMAKE_SOURCE_DIR}/executables" CACHE PATH "Path to code of executables.")
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/libraries" CACHE PATH "Path to code of libraries.")
set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/shaders" CACHE PATH "Path to code of shaders.")
set(PYTHON_INCLUDE_DIRS "${MINICONDA3_PATH}/include/python3.5m" CACHE PATH "Path to Python include directory.")
set(PYTHON_LIBRARIES "${MINICONDA3_PATH}/lib/libpython3.5m.so" CACHE PATH "Path of Python shared library.")
set(PYTHON_PACKAGES_PATH "${MINICONDA3_PATH}/lib/python3.5/site-packages" CACHE PATH "Path to Python packages.")

# Include cmake macros
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

# Build own version of GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "GLFW build examples." )
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "GLFW build tests.")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "GLFW build docs.")
set(GLFW_INSTALL OFF CACHE INTERNAL "GLFW install.")
set(GLFW_DOCUMENT_INTERNALS OFF CACHE INTERNAL "GLFW document internals.")
set(GLFW_USE_EGL OFF CACHE INTERNAL "GLFW use EGL.")
set(GLFW_USE_HYBRID_HPG OFF CACHE INTERNAL "GLFW use hybrid HPG.")
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE INTERNAL "MSCV runtime library dll.")
set(LIB_SUFFIX "" CACHE INTERNAL "Suffix of lib.")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "GLFW build shared libs.")
add_subdirectory(${SUBMODULESS_PATH}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
